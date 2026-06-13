#include "FtpDockableDlg.h"
#include "../PluginDefinition.h"
#include "../ConfigManager.h"
#include "../EventManager.h"
#include <commctrl.h>
#include <string>

extern NppData nppData;

static std::wstring s2ws_dock(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::string ws2s_dock(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

FtpDockableDlg::FtpDockableDlg() : DockingDlgInterface(IDD_FTP_DOCK), _hImageList(NULL) {
}

FtpDockableDlg::~FtpDockableDlg() {
    if (_hImageList) {
        ImageList_Destroy(_hImageList);
    }
}

void FtpDockableDlg::init(HINSTANCE hInst, HWND parent) {
    DockingDlgInterface::init(hInst, parent);
}

int FtpDockableDlg::getSystemIconIndex(const std::wstring& fileName, bool isDirectory) {
    SHFILEINFOW sfi = {0};
    DWORD flags = SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES;
    DWORD attr = isDirectory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    
    SHGetFileInfoW(fileName.c_str(), attr, &sfi, sizeof(sfi), flags);
    return sfi.iIcon;
}

void FtpDockableDlg::displayServerManager() {
    if (!_serverManager.isCreated()) {
        _serverManager.init(_hInst, _hParent);
    }
    _serverManager.doDialog();
}

void FtpDockableDlg::connectToServer(const std::string& profileId) {
    _currentProfileId = profileId;
    auto& profiles = ConfigManager::getInstance().getProfiles();
    for (const auto& p : profiles) {
        if (p.id == profileId) {
            logMessage(L"Łączenie z serwerem: " + s2ws_dock(p.host) + L"...");
            
            _ftpClient.setLogCallback([this](const std::string& msg) {
                this->logMessage(s2ws_dock(msg));
            });
            
            if (_ftpClient.connect(p.host, p.port, p.username, p.password, p.protocol)) {
                std::vector<RemoteFileInfo> files;
                if (_ftpClient.listDirectory(p.remoteDir, files)) {
                    HWND hTree = ::GetDlgItem(_hSelf, IDC_TREE_FILES);
                    TreeView_DeleteAllItems(hTree);
                    TVINSERTSTRUCTW tvs = {0};
                    tvs.hParent = TVI_ROOT;
                    tvs.hInsertAfter = TVI_LAST;
                    tvs.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
                    for (const auto& f : files) {
                        std::wstring wname = s2ws_dock(f.name);
                        int iconIndex = getSystemIconIndex(wname, f.isDirectory);
                        tvs.item.pszText = (LPWSTR)wname.c_str();
                        tvs.item.iImage = iconIndex;
                        tvs.item.iSelectedImage = iconIndex;
                        tvs.item.cChildren = f.isDirectory ? 1 : 0;
                        
                        std::string* fullPath = new std::string(p.remoteDir);
                        if(fullPath->empty() || fullPath->back() != '/') *fullPath += '/';
                        *fullPath += f.name;
                        tvs.item.lParam = (LPARAM)fullPath;
                        
                        TreeView_InsertItem(hTree, &tvs);
                    }
                }
                
                // Update tab name
                HWND hTab = ::GetDlgItem(_hSelf, IDC_TAB_SERVERS);
                TCITEMW tie = {0};
                tie.mask = TCIF_TEXT;
                std::wstring tabName = s2ws_dock(p.name);
                tie.pszText = (LPWSTR)tabName.c_str();
                TabCtrl_SetItem(hTab, 0, &tie);
            }
            break;
        }
    }
}

void FtpDockableDlg::logMessage(const std::wstring& msg) {
    HWND hEdit = ::GetDlgItem(_hSelf, IDC_EDIT_LOG);
    int len = ::GetWindowTextLength(hEdit);
    ::SendMessage(hEdit, EM_SETSEL, len, len);
    std::wstring line = msg + L"\r\n";
    ::SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)line.c_str());
}

INT_PTR CALLBACK FtpDockableDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Initialize system image list for TreeView
            SHFILEINFOW sfi = {0};
            _hImageList = (HIMAGELIST)SHGetFileInfoW(L"", 0, &sfi, sizeof(sfi), 
                SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            HWND hTree = ::GetDlgItem(_hSelf, IDC_TREE_FILES);
            TreeView_SetImageList(hTree, _hImageList, TVSIL_NORMAL);

            // Initialize Tabs
            HWND hTab = ::GetDlgItem(_hSelf, IDC_TAB_SERVERS);
            TCITEMW tie;
            tie.mask = TCIF_TEXT;
            tie.pszText = (LPWSTR)L"Offline";
            TabCtrl_InsertItem(hTab, 0, &tie);
            
            logMessage(L"Wtyczka gotowa.");
            return TRUE;
        }
        
        case WM_SIZE:
        {
            // Resize controls based on dock window size
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            
            HWND hTab = ::GetDlgItem(_hSelf, IDC_TAB_SERVERS);
            HWND hTree = ::GetDlgItem(_hSelf, IDC_TREE_FILES);
            HWND hLog = ::GetDlgItem(_hSelf, IDC_EDIT_LOG);
            
            ::MoveWindow(hTab, 2, 20, width - 4, 14, TRUE);
            ::MoveWindow(hTree, 2, 36, width - 4, height - 36 - 60, TRUE);
            ::MoveWindow(hLog, 2, height - 58, width - 4, 56, TRUE);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnmh = (LPNMHDR)lParam;
            if (lpnmh->idFrom == IDC_TREE_FILES) {
                if (lpnmh->code == TVN_ITEMEXPANDINGW || lpnmh->code == TVN_ITEMEXPANDINGA) {
                    LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                    if (pnmtv->action == TVE_EXPAND) {
                        // Sprawdzamy czy wezel juz ma wczytane dzieci
                        if (TreeView_GetChild(lpnmh->hwndFrom, pnmtv->itemNew.hItem) == NULL) {
                            std::string* pPath = (std::string*)pnmtv->itemNew.lParam;
                            if (pPath && !pPath->empty()) {
                                std::vector<RemoteFileInfo> subfiles;
                                if (_ftpClient.listDirectory(*pPath, subfiles)) {
                                    TVINSERTSTRUCTW tvs = {0};
                                    tvs.hParent = pnmtv->itemNew.hItem;
                                    tvs.hInsertAfter = TVI_LAST;
                                    tvs.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
                                    
                                    for (const auto& f : subfiles) {
                                        std::wstring wname = s2ws_dock(f.name);
                                        int iconIndex = getSystemIconIndex(wname, f.isDirectory);
                                        tvs.item.pszText = (LPWSTR)wname.c_str();
                                        tvs.item.iImage = iconIndex;
                                        tvs.item.iSelectedImage = iconIndex;
                                        tvs.item.cChildren = f.isDirectory ? 1 : 0;
                                        
                                        std::string* childPath = new std::string(*pPath);
                                        if(childPath->empty() || childPath->back() != '/') *childPath += '/';
                                        *childPath += f.name;
                                        tvs.item.lParam = (LPARAM)childPath;
                                        
                                        TreeView_InsertItem(lpnmh->hwndFrom, &tvs);
                                    }
                                }
                            }
                        }
                    }
                }
                else if (lpnmh->code == TVN_DELETEITEMW || lpnmh->code == TVN_DELETEITEMA) {
                    LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                    std::string* pPath = (std::string*)pnmtv->itemOld.lParam;
                    if (pPath) {
                        delete pPath;
                    }
                }
                else if (lpnmh->code == NM_DBLCLK) {
                    HTREEITEM hSel = TreeView_GetSelection(lpnmh->hwndFrom);
                    if (hSel) {
                        TVITEMW tvi = {0};
                        tvi.mask = TVIF_CHILDREN | TVIF_PARAM;
                        tvi.hItem = hSel;
                        TreeView_GetItem(lpnmh->hwndFrom, &tvi);
                        if (tvi.cChildren == 0 && tvi.lParam) { // file
                            std::string* pPath = (std::string*)tvi.lParam;
                            
                            // Generate local path
                            wchar_t tempPath[MAX_PATH];
                            GetTempPathW(MAX_PATH, tempPath);
                            std::wstring localPath = std::wstring(tempPath) + L"PandaFTP_";
                            
                            // Extract filename from pPath
                            size_t pos = pPath->find_last_of('/');
                            std::string filename = (pos == std::string::npos) ? *pPath : pPath->substr(pos + 1);
                            localPath += s2ws_dock(filename);
                            
                            logMessage(L"Pobieranie: " + s2ws_dock(filename) + L"...");
                            
                            // Download
                            if (_ftpClient.downloadFile(*pPath, ws2s_dock(localPath))) {
                                EventManager::getInstance().registerDownloadedFile(localPath, _currentProfileId, *pPath);
                                EventManager::getInstance().openLocalFileInEditor(localPath);
                                logMessage(L"Pobrano plik pomyślnie.");
                            } else {
                                logMessage(L"Błąd pobierania pliku!");
                            }
                        }
                    }
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (wmId == IDC_BTN_MGR) {
                displayServerManager();
                return TRUE;
            }
            else if (wmId == IDC_BTN_DISCONNECT) {
                _ftpClient.disconnect();
                TreeView_DeleteAllItems(::GetDlgItem(_hSelf, IDC_TREE_FILES));
                logMessage(L"Rozłączono.");
                return TRUE;
            }
            else if (wmId == IDC_BTN_UPLOAD) {
                if (_currentProfileId.empty()) {
                    logMessage(L"Brak aktywnego połączenia!");
                    return TRUE;
                }
                
                // Get current file path from Notepad++
                wchar_t currentPath[MAX_PATH];
                ::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)currentPath);
                
                if (wcslen(currentPath) == 0 || wcscmp(currentPath, L"new 1") == 0 || wcsstr(currentPath, L"new ") == currentPath) {
                    logMessage(L"Zapisz najpierw plik lokalnie na dysku przed wysłaniem!");
                    return TRUE;
                }
                
                // Find selected folder in TreeView to upload to
                HWND hTree = ::GetDlgItem(_hSelf, IDC_TREE_FILES);
                HTREEITEM hSel = TreeView_GetSelection(hTree);
                std::string targetDir = "/";
                
                if (hSel) {
                    TVITEMW tvi = {0};
                    tvi.mask = TVIF_PARAM | TVIF_CHILDREN;
                    tvi.hItem = hSel;
                    TreeView_GetItem(hTree, &tvi);
                    
                    if (tvi.lParam) {
                        std::string* pPath = (std::string*)tvi.lParam;
                        if (tvi.cChildren > 0) { // It's a directory
                            targetDir = *pPath;
                        } else { // It's a file, get its parent directory
                            size_t pos = pPath->find_last_of('/');
                            if (pos != std::string::npos) {
                                targetDir = pPath->substr(0, pos);
                            }
                        }
                    }
                }
                
                // Extract filename from currentPath
                std::wstring wPath(currentPath);
                size_t slashPos = wPath.find_last_of(L"\\/");
                std::wstring wFileName = (slashPos == std::wstring::npos) ? wPath : wPath.substr(slashPos + 1);
                
                std::string localUtf8 = ws2s_dock(wPath);
                std::string remoteFileName = ws2s_dock(wFileName);
                
                if(targetDir.empty() || targetDir.back() != '/') targetDir += '/';
                std::string remoteFullPath = targetDir + remoteFileName;
                
                logMessage(L"Wysyłanie: " + wFileName + L"...");
                
                if (_ftpClient.uploadFile(localUtf8, remoteFullPath)) {
                    EventManager::getInstance().registerDownloadedFile(wPath, _currentProfileId, remoteFullPath);
                    logMessage(L"Plik wysłany pomyślnie.");
                    
                    // Odswiez wezel docelowy
                    if (hSel) {
                        if (TreeView_GetChild(hTree, hSel) != NULL) {
                            // Jesli folder byl rozwiniety, najprosciej zwinac go i rozwinac (odswiezenie)
                            TreeView_Expand(hTree, hSel, TVE_COLLAPSE | TVE_COLLAPSERESET);
                            TreeView_Expand(hTree, hSel, TVE_EXPAND);
                        }
                    }
                } else {
                    logMessage(L"Błąd wysyłania pliku!");
                }
                return TRUE;
            }
            break;
        }

        default:
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
    return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}
