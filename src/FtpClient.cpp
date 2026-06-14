#include "FtpClient.h"
#include <iostream>
#include <sstream>
#include <iomanip>

static std::string urlEncode(CURL* curl, const std::string& str) {
    if (str.empty()) return "";
    
    // We only want to encode characters that are invalid in URLs, 
    // but KEEP the forward slashes '/' as they are path separators.
    std::string result = "";
    std::string current_segment = "";
    
    for (char c : str) {
        if (c == '/') {
            if (!current_segment.empty()) {
                char* escaped = curl_easy_escape(curl, current_segment.c_str(), (int)current_segment.length());
                if (escaped) {
                    result += escaped;
                    curl_free(escaped);
                }
                current_segment = "";
            }
            result += "/";
        } else {
            current_segment += c;
        }
    }
    
    if (!current_segment.empty()) {
        char* escaped = curl_easy_escape(curl, current_segment.c_str(), (int)current_segment.length());
        if (escaped) {
            result += escaped;
            curl_free(escaped);
        }
    }
    
    return result;
}

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* mem = (std::string*)userp;
    mem->append((char*)contents, realsize);
    return realsize;
}

static int DebugCallback(CURL* /*handle*/, curl_infotype type, char* data, size_t size, void* userp) {
    if (type == CURLINFO_TEXT || type == CURLINFO_HEADER_IN || type == CURLINFO_HEADER_OUT) {
        FtpClient* client = (FtpClient*)userp;
        std::string text(data, size);
        if (!text.empty() && text.back() == '\n') text.pop_back();
        if (!text.empty() && text.back() == '\r') text.pop_back();
        if (!text.empty()) {
            client->log("[CURL] " + text);
        }
    }
    return 0;
}

FtpClient::FtpClient() : curl(nullptr) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

FtpClient::~FtpClient() {
    disconnect();
    curl_global_cleanup();
}

void FtpClient::log(const std::string& msg) {
    if (logCallback) logCallback(msg);
}

bool FtpClient::connect(const std::string& host, int port, const std::string& username, const std::string& password, const std::string& protocol) {
    disconnect();
    
    curl = curl_easy_init();
    if (!curl) {
        log("Błąd: Nie można zainicjować libcurl.");
        return false;
    }
    
    currentUsername = username;
    currentPassword = password;
    
    std::string scheme = (protocol == "SFTP") ? "sftp://" : "ftp://";
    baseUrl = scheme + host + ":" + std::to_string(port);
    
    // Perform a test connection / directory list to verify
    std::string url = baseUrl;
    // Don't append trailing slash immediately as it breaks some SFTP implementations during login
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    if (!username.empty() || !password.empty()) {
        std::string userpwd = username + ":" + password;
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd.c_str());
    }
    
    // Ignore SSL/SSH verification for self-signed servers for simplicity
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    // Enable debug output
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, DebugCallback);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, this);
    
    // Some FTP servers require passive mode
    curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 1L);
    
    // Explicitly allow any SSH auth (password or keyboard-interactive)
    curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_ANY);
    
    log("Połączono z serwerem " + protocol + " pomyślnie.");
    return true;
}

void FtpClient::disconnect() {
    if (curl) {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }
    baseUrl.clear();
    currentUsername.clear();
    currentPassword.clear();
}

bool FtpClient::listDirectory(const std::string& path, std::vector<RemoteFileInfo>& outFiles) {
    if (!curl) return false;
    
    std::string url = baseUrl;
    if (!path.empty() && path != "/") {
        std::string safePath = urlEncode(curl, path);
        url += (safePath.front() == '/' ? safePath : "/" + safePath);
    }
    if (url.back() != '/') url += '/';
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);
    
    std::string chunk;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log("Błąd pobierania listy plików: " + std::string(curl_easy_strerror(res)));
        return false;
    }
    
    std::istringstream stream(chunk);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == "." || line == "..") continue;
        if (!line.empty()) {
            RemoteFileInfo rfi;
            rfi.name = line;
            // W trybie DIRLISTONLY libcurl zwraca tylko nazwy, więc nie wiemy czy to katalog
            // bez komendy CUSTOMREQUEST. Dla uproszczenia załóżmy plik, chyba że brak rozszerzenia
            rfi.isDirectory = (line.find('.') == std::string::npos);
            rfi.size = 0;
            outFiles.push_back(rfi);
        }
    }
    
    log("Lista plików została pobrana.");
    return true;
}

bool FtpClient::downloadFile(const std::string& remotePath, const std::string& localPath) {
    if (!curl) return false;
    
    FILE* file = nullptr;
    fopen_s(&file, localPath.c_str(), "wb");
    if (!file) {
        log("Błąd: Nie można otworzyć pliku lokalnego do zapisu.");
        return false;
    }
    
    std::string url = baseUrl;
    if (!remotePath.empty()) {
        std::string safePath = urlEncode(curl, remotePath);
        url += (safePath.front() == '/' ? safePath : "/" + safePath);
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 0L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    
    CURLcode res = curl_easy_perform(curl);
    fclose(file);
    
    if (res != CURLE_OK) {
        log("Błąd pobierania pliku: " + std::string(curl_easy_strerror(res)));
        return false;
    }
    
    log("Pobrano plik pomyślnie.");
    return true;
}

bool FtpClient::uploadFile(const std::string& localPath, const std::string& remotePath) {
    if (!curl) return false;
    
    FILE* file = nullptr;
    fopen_s(&file, localPath.c_str(), "rb");
    if (!file) {
        log("Błąd: Nie można otworzyć pliku lokalnego do odczytu.");
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    std::string url = baseUrl;
    if (!remotePath.empty()) {
        std::string safePath = urlEncode(curl, remotePath);
        url += (safePath.front() == '/' ? safePath : "/" + safePath);
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, file);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, fsize);
    
    CURLcode res = curl_easy_perform(curl);
    fclose(file);
    
    if (res != CURLE_OK) {
        log("Błąd wysyłania pliku: " + std::string(curl_easy_strerror(res)));
        return false;
    }
    
    log("Wysłano plik pomyślnie.");
    return true;
}
