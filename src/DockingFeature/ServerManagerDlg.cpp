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

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
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

            // Tłumaczenie UI na język angielski, jeśli używamy innej wersji N++
            wchar_t langPath[MAX_PATH];
            ::SendMessage(nppData._nppHandle, NPPM_GETNATIVELANGFILENAME, MAX_PATH, (LPARAM)langPath);
            std::wstring wLangPath(langPath);
            if (wLangPath.find(L"polish.xml") == std::wstring::npos && wLangPath.find(L"polski") == std::wstring::npos) {
                ::SetWindowTextW(_hSelf, L"Server Manager");
                ::SetDlgItemTextW(_hSelf, IDC_BTN_NEW_PROFILE, L"New");
                ::SetDlgItemTextW(_hSelf, IDC_BTN_DEL_PROFILE, L"Delete");
                ::SetDlgItemTextW(_hSelf, IDC_BTN_CONNECT, L"Connect");
                ::SetDlgItemTextW(_hSelf, IDC_BTN_SAVE, L"Save");
                ::SetDlgItemTextW(_hSelf, IDC_BTN_CLOSE, L"Cancel");
                
                // Trzeba by było również przetłumaczyć Static texty, ale to wymagałoby ich własnych IDC_ zamiast IDC_STATIC
                // Ze względu na uproszczenie, zmieniamy tylko przyciski i tytuł
            }

            refreshProfileList();
            return TRUE;
        }

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            // Gdy wpisujemy cokolwiek w polach edycyjnych formularza
            if ((wmEvent == EN_CHANGE) && 
                (wmId == IDC_EDIT_PROFILENAME || wmId == IDC_EDIT_HOST || 
                 wmId == IDC_EDIT_USER || wmId == IDC_EDIT_PASS || 
                 wmId == IDC_EDIT_REMOTEDIR || wmId == IDC_EDIT_PORT)) {
                
                // Jeśli nie mamy wybranego żadnego elementu na liście (co oznacza tworzenie nowego profilu)
                HWND hList = ::GetDlgItem(_hSelf, IDC_LIST_PROFILES);
                int index = (int)::SendMessage(hList, LB_GETCURSEL, 0, 0);
                if (index == LB_ERR) {
                    // Wymuszamy, by ID było puste
                    currentProfileId = "";
                }
            }

            if (wmId == IDC_BTN_CLOSE || wmId == IDCANCEL) {
                display(false);
                return TRUE;
            }
            else if (wmId == IDC_BTN_NEW_PROFILE) {
                clearForm();
                currentProfileId = ""; // Wyczyść ID by traktować to jako nowy profil
                // Odznacz wszystko na liście, aby było widać, że to nowy profil
                HWND hList = ::GetDlgItem(_hSelf, IDC_LIST_PROFILES);
                ::SendMessage(hList, LB_SETCURSEL, (WPARAM)-1, 0);
            }
            else if (wmId == IDC_BTN_DEL_PROFILE) {
                if (!currentProfileId.empty()) {
                    ConfigManager::getInstance().removeProfile(currentProfileId);
                    
                    // Zapisz po usunięciu
                    wchar_t configDir[MAX_PATH];
                    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configDir);
                    std::wstring configPath = std::wstring(configDir) + L"\\PandaFTP.json";
                    ConfigManager::getInstance().saveConfig(configPath);
                    
                    currentProfileId = "";
                    clearForm();
                    refreshProfileList();
                }
            }
            else if (wmId == IDC_BTN_SAVE) {
                if (saveCurrentProfile()) {
                    refreshProfileList();
                    // Zaznacz nowo zapisany profil na liście
                    HWND hList = ::GetDlgItem(_hSelf, IDC_LIST_PROFILES);
                    int count = (int)::SendMessage(hList, LB_GETCOUNT, 0, 0);
                    auto& profiles = ConfigManager::getInstance().getProfiles();
                    for (int i = 0; i < count; ++i) {
                        int profileIndex = (int)::SendMessage(hList, LB_GETITEMDATA, i, 0);
                        if (profileIndex >= 0 && profileIndex < (int)profiles.size()) {
                            if (profiles[profileIndex].id == currentProfileId) {
                                ::SendMessage(hList, LB_SETCURSEL, i, 0);
                                break;
                            }
                        }
                    }
                }
            }
            else if (wmId == IDC_BTN_CONNECT) {
                if (saveCurrentProfile()) {
                    ftpDockDlg.connectToServer(currentProfileId);
                    display(false); // Close manager
                }
                return TRUE;
            }
            else if (wmId == IDC_LIST_PROFILES && wmEvent == LBN_SELCHANGE) {
                HWND hList = ::GetDlgItem(_hSelf, IDC_LIST_PROFILES);
                int listIndex = (int)::SendMessage(hList, LB_GETCURSEL, 0, 0);
                if (listIndex != LB_ERR) {
                    int profileIndex = (int)::SendMessage(hList, LB_GETITEMDATA, listIndex, 0);
                    auto& profiles = ConfigManager::getInstance().getProfiles();
                    if (profileIndex >= 0 && profileIndex < static_cast<int>(profiles.size())) {
                        loadProfileData(profiles[profileIndex].id);
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
    for (size_t i = 0; i < profiles.size(); ++i) {
        std::wstring wname = s2ws(profiles[i].name);
        int listIndex = (int)::SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)wname.c_str());
        ::SendMessage(hList, LB_SETITEMDATA, listIndex, (LPARAM)i);
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

bool ServerManagerDlg::saveCurrentProfile() {
    wchar_t buf[256];
    ServerProfile p;
    
    // Upewnijmy się, że nazwa profilu nie jest pusta, żeby nie nadpisywać śmieciami
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_PROFILENAME, buf, 256);
    if (wcslen(buf) == 0) {
        MessageBoxW(_hSelf, L"Nazwa profilu nie może być pusta!", L"Błąd", MB_OK | MB_ICONERROR);
        return false;
    }
    p.name = ws2s(buf);
    
    if (currentProfileId.empty()) {
        // Wygeneruj NOWE unikalne ID, dodając trochę losowości
        currentProfileId = std::to_string(GetTickCount64()) + "_" + std::to_string(rand());
    }
    p.id = currentProfileId;
    
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_HOST, buf, 256); p.host = trim(ws2s(buf));
    p.port = ::GetDlgItemInt(_hSelf, IDC_EDIT_PORT, NULL, FALSE);
    
    HWND hCombo = ::GetDlgItem(_hSelf, IDC_COMBO_PROTOCOL);
    int idx = (int)::SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    p.protocol = (idx == 1) ? "SFTP" : "FTP";
    
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_USER, buf, 256); p.username = trim(ws2s(buf));
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_PASS, buf, 256); p.password = trim(ws2s(buf));
    ::GetDlgItemTextW(_hSelf, IDC_EDIT_REMOTEDIR, buf, 256); p.remoteDir = trim(ws2s(buf));
    
    ConfigManager::getInstance().addProfile(p);
    
    // Save to disk
    wchar_t configDir[MAX_PATH];
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configDir);
    std::wstring configPath = std::wstring(configDir) + L"\\PandaFTP.json";
    ConfigManager::getInstance().saveConfig(configPath);
    
    return true;
}

void ServerManagerDlg::clearForm() {
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_PROFILENAME, L"");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_HOST, L"");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_PORT, L"21");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_USER, L"");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_PASS, L"");
    ::SetDlgItemTextW(_hSelf, IDC_EDIT_REMOTEDIR, L"/");
}
