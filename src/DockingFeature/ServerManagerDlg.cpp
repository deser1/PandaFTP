#include "ServerManagerDlg.h"
#include "FtpDockableDlg.h"
#include "../PluginDefinition.h"
#include <windows.h>
#include <string>

extern FtpDockableDlg ftpDockDlg;
extern NppData nppData;

// Helper to convert std::string to std::wstring
static std::wstring s2ws(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Helper to convert std::wstring to std::string
static std::string ws2s(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

INT_PTR CALLBACK ServerManagerDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Init protocol combobox
            HWND hCombo = ::GetDlgItem(_hSelf, IDC_COMBO_PROTOCOL);
            ::SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"FTP");
            ::SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"SFTP");
            ::SendMessage(hCombo, CB_SETCURSEL, 0, 0);

            refreshProfileList();
            return TRUE;
        }

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            if (wmId == IDC_BTN_CLOSE || wmId == IDCANCEL) {
                display(false);
                return TRUE;
            }
            else if (wmId == IDC_BTN_NEW_PROFILE) {
                clearForm();
                currentProfileId = "";
            }
            else if (wmId == IDC_BTN_SAVE) {
                saveCurrentProfile();
                refreshProfileList();
            }
            else if (wmId == IDC_BTN_CONNECT) {
                saveCurrentProfile();
                ftpDockDlg.connectToServer(currentProfileId);
                display(false); // Close manager
                return TRUE;
            }
            else if (wmId == IDC_LIST_PROFILES && wmEvent == LBN_SELCHANGE) {
                HWND hList = ::GetDlgItem(_hSelf, IDC_LIST_PROFILES);
                int index = (int)::SendMessage(hList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR) {
                    auto& profiles = ConfigManager::getInstance().getProfiles();
                    if (index >= 0 && index < static_cast<int>(profiles.size())) {
                        loadProfileData(profiles[index].id);
                    }
                }
            }
            return FALSE;
        }

        default:
            return FALSE;
    }
}

void ServerManagerDlg::refreshProfileList() {
    HWND hList = ::GetDlgItem(_hSelf, IDC_LIST_PROFILES);
    ::SendMessage(hList, LB_RESETCONTENT, 0, 0);
    
    auto& profiles = ConfigManager::getInstance().getProfiles();
    for (const auto& p : profiles) {
        std::wstring wname = s2ws(p.name);
        ::SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)wname.c_str());
    }
}

void ServerManagerDlg::loadProfileData(const std::string& id) {
    currentProfileId = id;
    auto& profiles = ConfigManager::getInstance().getProfiles();
    for (const auto& p : profiles) {
        if (p.id == id) {
            ::SetDlgItemTextW(_hSelf, IDC_EDIT_PROFILENAME, s2ws(p.name).c_str());
            ::SetDlgItemTextW(_hSelf, IDC_EDIT_HOST, s2ws(p.host).c_str());
            ::SetDlgItemInt(_hSelf, IDC_EDIT_PORT, p.port, FALSE);
            
            HWND hCombo = ::GetDlgItem(_hSelf, IDC_COMBO_PROTOCOL);
            int idx = (p.protocol == "SFTP") ? 1 : 0;
            ::SendMessage(hCombo, CB_SETCURSEL, idx, 0);
            
            ::SetDlgItemTextW(_hSelf, IDC_EDIT_USER, s2ws(p.username).c_str());
            ::SetDlgItemTextW(_hSelf, IDC_EDIT_PASS, s2ws(p.password).c_str());
            ::SetDlgItemTextW(_hSelf, IDC_EDIT_REMOTEDIR, s2ws(p.remoteDir).c_str());
            break;
        }
    }
}

void ServerManagerDlg::saveCurrentProfile() {
    wchar_t buf[256];
    ServerProfile p;
    
    if (currentProfileId.empty()) {
        currentProfileId = std::to_string(GetTickCount64()); // Simple unique ID
    }
    p.id = currentProfileId;
    
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_PROFILENAME, buf, 256); p.name = ws2s(buf);
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_HOST, buf, 256); p.host = ws2s(buf);
    p.port = ::GetDlgItemInt(_hSelf, IDC_EDIT_PORT, NULL, FALSE);
    
    HWND hCombo = ::GetDlgItem(_hSelf, IDC_COMBO_PROTOCOL);
    int idx = (int)::SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    p.protocol = (idx == 1) ? "SFTP" : "FTP";
    
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_USER, buf, 256); p.username = ws2s(buf);
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_PASS, buf, 256); p.password = ws2s(buf);
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_REMOTEDIR, buf, 256); p.remoteDir = ws2s(buf);
    
    ConfigManager::getInstance().addProfile(p);
    
    // Save to disk
    wchar_t configDir[MAX_PATH];
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configDir);
    std::wstring configPath = std::wstring(configDir) + L"\\PandaFTP.json";
    ConfigManager::getInstance().saveConfig(configPath);
}

void ServerManagerDlg::clearForm() {
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_PROFILENAME, L"");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_HOST, L"");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_PORT, L"21");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_USER, L"");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_PASS, L"");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_REMOTEDIR, L"/");
}
