#ifndef PANDAFTP_EVENTS_H
#define PANDAFTP_EVENTS_H

#include <string>
#include <map>

// Mapowanie lokalnej ścieżki pliku (pobranego z serwera) na jego oryginalną zdalną ścieżkę i profil serwera
struct RemoteFileMapping {
    std::string profileId;
    std::string remotePath;
};

class EventManager {
public:
    static EventManager& getInstance() {
        static EventManager instance;
        return instance;
    }

    void onFileSaved(const std::wstring& localFilePath);
    void onFileClosed(const std::wstring& localFilePath);
    void registerDownloadedFile(const std::wstring& localFilePath, const std::string& profileId, const std::string& remotePath);
    void openLocalFileInEditor(const std::wstring& localFilePath);

private:
    EventManager() = default;
    ~EventManager() = default;

    std::map<std::wstring, RemoteFileMapping> activeFiles;
};

#endif // PANDAFTP_EVENTS_H
