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
    // Check if already connected
    for (size_t i = 0; i < _connections.size(); ++i) {
        if (_connections[i].profileId == profileId) {
            switchToConnection((int)i);
            return;
        }
    }

    auto& profiles = ConfigManager::getInstance().getProfiles();
    for (const auto& p : profiles) {
        if (p.id == profileId) {
            logMessage(L"Łączenie z serwerem: " + s2ws_dock(p.host) + L"...");
            
            auto client = std::make_shared<FtpClient>();
            client->setLogCallback([this](const std::string& msg) {
                this->logMessage(s2ws_dock(msg));
            });
            
            if (client->connect(p.host, p.port, p.username, p.password, p.protocol)) {
                // If this is the first connection, clear the "Offline" tab
                HWND hTab = ::GetDlgItem(_hSelf, IDC_TAB_SERVERS);
                if (_connections.empty()) {
                    TabCtrl_DeleteAllItems(hTab);
                }

                // Create new TreeView
                RECT rc;
                HWND hDefaultTree = ::GetDlgItem(_hSelf, IDC_TREE_FILES);
                ::GetWindowRect(hDefaultTree, &rc);
                MapWindowPoints(NULL, _hSelf, (LPPOINT)&rc, 2);
                
                HWND hNewTree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
                    WS_CHILD | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
                    rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                    _hSelf, (HMENU)0, _hInst, NULL);
                
                TreeView_SetImageList(hNewTree, _hImageList, TVSIL_NORMAL);
                
                // Populate TreeView
                std::vector<RemoteFileInfo> files;
                if (client->listDirectory(p.remoteDir, files)) {
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
                        
                        TreeView_InsertItem(hNewTree, &tvs);
                    }
                }
                
                // Add tab
                TCITEMW tie = {0};
                tie.mask = TCIF_TEXT;
                std::wstring tabName = s2ws_dock(p.name);
                tie.pszText = (LPWSTR)tabName.c_str();
                int newIndex = (int)_connections.size();
                TabCtrl_InsertItem(hTab, newIndex, &tie);
                
                // Store connection
                ServerConnection conn;
                conn.profileId = p.id;
                conn.profileName = p.name;
                conn.client = client;
                conn.hTree = hNewTree;
                _connections.push_back(conn);
                
                switchToConnection(newIndex);
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

void FtpDockableDlg::switchToConnection(int index) {
    HWND hTab = ::GetDlgItem(_hSelf, IDC_TAB_SERVERS);
    HWND hDefaultTree = ::GetDlgItem(_hSelf, IDC_TREE_FILES);
    
    // Hide all trees
    ::ShowWindow(hDefaultTree, SW_HIDE);
    for (auto& conn : _connections) {
        ::ShowWindow(conn.hTree, SW_HIDE);
    }
    
    if (index >= 0 && index < (int)_connections.size()) {
        _activeConnectionIndex = index;
        ::ShowWindow(_connections[index].hTree, SW_SHOW);
        TabCtrl_SetCurSel(hTab, index);
    } else {
        _activeConnectionIndex = -1;
        ::ShowWindow(hDefaultTree, SW_SHOW);
        if (_connections.empty()) {
            TabCtrl_DeleteAllItems(hTab);
            TCITEMW tie = {0};
            tie.mask = TCIF_TEXT;
            tie.pszText = (LPWSTR)L"Offline";
            TabCtrl_InsertItem(hTab, 0, &tie);
            TabCtrl_SetCurSel(hTab, 0);
        }
    }
}

void FtpDockableDlg::closeConnection(int index) {
    if (index < 0 || index >= (int)_connections.size()) return;
    
    // Disconnect and destroy tree
    _connections[index].client->disconnect();
    ::DestroyWindow(_connections[index].hTree);
    
    // Remove tab
    HWND hTab = ::GetDlgItem(_hSelf, IDC_TAB_SERVERS);
    TabCtrl_DeleteItem(hTab, index);
    
    // Remove from vector
    _connections.erase(_connections.begin() + index);
    
    // Switch to adjacent tab or offline
    int newIndex = index - 1;
    if (newIndex < 0 && !_connections.empty()) newIndex = 0;
    
    switchToConnection(newIndex);
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
            HWND hDefaultTree = ::GetDlgItem(_hSelf, IDC_TREE_FILES);
            HWND hLog = ::GetDlgItem(_hSelf, IDC_EDIT_LOG);
            
            ::MoveWindow(hTab, 2, 22, width - 4, 24, TRUE);
            ::MoveWindow(hDefaultTree, 2, 48, width - 4, height - 48 - 60, TRUE);
            for (auto& conn : _connections) {
                ::MoveWindow(conn.hTree, 2, 48, width - 4, height - 48 - 60, TRUE);
            }
            ::MoveWindow(hLog, 2, height - 58, width - 4, 56, TRUE);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnmh = (LPNMHDR)lParam;
            
            // Obsługa przełączania zakładek
            if (lpnmh->idFrom == IDC_TAB_SERVERS && lpnmh->code == TCN_SELCHANGE) {
                int index = TabCtrl_GetCurSel(lpnmh->hwndFrom);
                switchToConnection(index);
                return TRUE;
            }
            
            // Sprawdzanie czy powiadomienie pochodzi z jednego z naszych drzew plików
            bool isOurTree = false;
            int treeIndex = -1;
            for (int i = 0; i < (int)_connections.size(); ++i) {
                if (lpnmh->hwndFrom == _connections[i].hTree) {
                    isOurTree = true;
                    treeIndex = i;
                    break;
                }
            }
            
            if (isOurTree) {
                if (lpnmh->code == TVN_ITEMEXPANDINGW || lpnmh->code == TVN_ITEMEXPANDINGA) {
                    LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                    if (pnmtv->action == TVE_EXPAND) {
                        if (TreeView_GetChild(lpnmh->hwndFrom, pnmtv->itemNew.hItem) == NULL) {
                            std::string* pPath = (std::string*)pnmtv->itemNew.lParam;
                            if (pPath && !pPath->empty()) {
                                std::vector<RemoteFileInfo> subfiles;
                                if (_connections[treeIndex].client->listDirectory(*pPath, subfiles)) {
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
                            
                            wchar_t tempPath[MAX_PATH];
                            GetTempPathW(MAX_PATH, tempPath);
                            std::wstring localPath = std::wstring(tempPath) + L"PandaFTP_";
                            
                            size_t pos = pPath->find_last_of('/');
                            std::string filename = (pos == std::string::npos) ? *pPath : pPath->substr(pos + 1);
                            localPath += s2ws_dock(filename);
                            
                            logMessage(L"Pobieranie: " + s2ws_dock(filename) + L"...");
                            
                            if (_connections[treeIndex].client->downloadFile(*pPath, ws2s_dock(localPath))) {
                                EventManager::getInstance().registerDownloadedFile(localPath, _connections[treeIndex].profileId, *pPath);
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
                if (_activeConnectionIndex >= 0) {
                    closeConnection(_activeConnectionIndex);
                    logMessage(L"Rozłączono.");
                }
                return TRUE;
            }
            else if (wmId == IDC_BTN_UPLOAD) {
                if (_activeConnectionIndex < 0) {
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
                
                // Find selected folder in active TreeView to upload to
                HWND hTree = _connections[_activeConnectionIndex].hTree;
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
                
                if (_connections[_activeConnectionIndex].client->uploadFile(localUtf8, remoteFullPath)) {
                    EventManager::getInstance().registerDownloadedFile(wPath, _connections[_activeConnectionIndex].profileId, remoteFullPath);
                    logMessage(L"Plik wysłany pomyślnie.");
                    
                    // Odswiez wezel docelowy
                    if (hSel) {
                        if (TreeView_GetChild(hTree, hSel) != NULL) {
                            TreeView_Expand(hTree, hSel, TVE_COLLAPSE | TVE_COLLAPSERESET);
                            TreeView_Expand(hTree, hSel, TVE_EXPAND);
                        }
                    }
                } else {
                    logMessage(L"Błąd wysyłania pliku!");
                }
                return TRUE;
            }
            else if (wmId == IDC_BTN_REFRESH) {
                if (_activeConnectionIndex >= 0) {
                    HWND hTree = _connections[_activeConnectionIndex].hTree;
                    HTREEITEM hSel = TreeView_GetSelection(hTree);
                    if (hSel) {
                        TreeView_Expand(hTree, hSel, TVE_COLLAPSE | TVE_COLLAPSERESET);
                        TreeView_Expand(hTree, hSel, TVE_EXPAND);
                        logMessage(L"Odświeżono katalog.");
                    }
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
