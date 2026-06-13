#include "EventManager.h"
#include "Notepad_plus_msgs.h"
#include "PluginDefinition.h"
#include "FtpClient.h"
#include "ConfigManager.h"
#include <windows.h>
#include <iostream>

extern NppData nppData;

// Helper
static std::string ws2s(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void EventManager::registerDownloadedFile(const std::wstring& localFilePath, const std::string& profileId, const std::string& remotePath) {
    activeFiles[localFilePath] = { profileId, remotePath };
}

void EventManager::openLocalFileInEditor(const std::wstring& localFilePath) {
    ::SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)localFilePath.c_str());
}

void EventManager::onFileSaved(const std::wstring& localFilePath) {
    auto it = activeFiles.find(localFilePath);
    if (it != activeFiles.end()) {
        const auto& mapping = it->second;
        
        // Find profile
        ServerProfile profile;
        bool found = false;
        for (const auto& p : ConfigManager::getInstance().getProfiles()) {
            if (p.id == mapping.profileId) {
                profile = p;
                found = true;
                break;
            }
        }
        
        if (found) {
            // Initiate background upload
            // In a complete implementation, this should be done in a separate thread
            // to avoid freezing Notepad++ while uploading.
            FtpClient client;
            if (client.connect(profile.host, profile.port, profile.username, profile.password, profile.protocol)) {
                std::string localUtf8 = ws2s(localFilePath);
                bool success = client.uploadFile(localUtf8, mapping.remotePath);
                
                if (success) {
                    // Update log or status bar in Notepad++
                    // e.g. ::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)L"PandaFTP: Plik zapisany na serwerze");
                }
            }
        }
    }
}

void EventManager::onFileClosed(const std::wstring& localFilePath) {
    auto it = activeFiles.find(localFilePath);
    if (it != activeFiles.end()) {
        // Usuwamy fizyczny plik z dysku z folderu Temp
        ::DeleteFileW(localFilePath.c_str());
        
        // Usuwamy wpis z naszej pamieci RAM
        activeFiles.erase(it);
    }
}
