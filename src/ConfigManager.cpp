#include "ConfigManager.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <algorithm>

using json = nlohmann::json;

void ConfigManager::loadConfig(const std::wstring& configFilePath) {
    std::ifstream file(configFilePath);
    if (!file.is_open()) return;

    try {
        json j;
        file >> j;

        profiles.clear();
        for (const auto& item : j["profiles"]) {
            ServerProfile profile;
            profile.id = item.value("id", "");
            profile.name = item.value("name", "Unknown");
            profile.host = item.value("host", "");
            profile.port = item.value("port", 21);
            profile.protocol = item.value("protocol", "FTP");
            profile.username = item.value("username", "");
            profile.password = item.value("password", ""); // TODO: Decrypt
            profile.remoteDir = item.value("remoteDir", "/");
            profiles.push_back(profile);
        }
    }
    catch (...) {
        // Handle parsing errors
    }
}

void ConfigManager::saveConfig(const std::wstring& configFilePath) {
    json j;
    j["profiles"] = json::array();

    for (const auto& profile : profiles) {
        json item;
        item["id"] = profile.id;
        item["name"] = profile.name;
        item["host"] = profile.host;
        item["port"] = profile.port;
        item["protocol"] = profile.protocol;
        item["username"] = profile.username;
        item["password"] = profile.password; // TODO: Encrypt
        item["remoteDir"] = profile.remoteDir;
        j["profiles"].push_back(item);
    }

    std::ofstream file(configFilePath);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

void ConfigManager::addProfile(const ServerProfile& profile) {
    for (auto& p : profiles) {
        if (p.id == profile.id) {
            p = profile; // Update existing
            return;
        }
    }
    profiles.push_back(profile);
}

void ConfigManager::removeProfile(const std::string& id) {
    profiles.erase(
        std::remove_if(profiles.begin(), profiles.end(),
            [&id](const ServerProfile& p) { return p.id == id; }),
        profiles.end()
    );
}
