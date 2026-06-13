#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <vector>

struct ServerProfile {
    std::string id;
    std::string name;
    std::string host;
    int port;
    std::string protocol; // "FTP" or "SFTP"
    std::string username;
    std::string password; // Should be encrypted in a real scenario
    std::string remoteDir;
};

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    void loadConfig(const std::wstring& configFilePath);
    void saveConfig(const std::wstring& configFilePath);

    std::vector<ServerProfile>& getProfiles() { return profiles; }
    void addProfile(const ServerProfile& profile);
    void removeProfile(const std::string& id);

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    std::vector<ServerProfile> profiles;
};

#endif // CONFIGMANAGER_H
