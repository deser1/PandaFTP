#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <string>
#include <vector>
#include <functional>
#include <curl/curl.h>

struct RemoteFileInfo {
    std::string name;
    bool isDirectory;
    long long size;
};

class FtpClient {
public:
    FtpClient();
    ~FtpClient();
    
    // allow debug callback access
    void log(const std::string& msg);

    bool connect(const std::string& host, int port, const std::string& username, const std::string& password, const std::string& protocol);
    void disconnect();

    bool listDirectory(const std::string& path, std::vector<RemoteFileInfo>& outFiles);
    bool downloadFile(const std::string& remotePath, const std::string& localPath);
    bool uploadFile(const std::string& localPath, const std::string& remotePath);

    void setLogCallback(std::function<void(const std::string&)> callback) {
        logCallback = callback;
    }

    const std::string& getCurrentPassword() const { return currentPassword; }

private:
    CURL* curl;
    std::string baseUrl;
    std::string currentUsername;
    std::string currentPassword;

    std::function<void(const std::string&)> logCallback;
};

#endif // FTPCLIENT_H
