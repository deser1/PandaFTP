#ifndef FTPDOCKABLEDLG_H
#define FTPDOCKABLEDLG_H

#include "DockingDlgInterface.h"
#include "ServerManagerDlg.h"
#include "resource.h"
#include "../FtpClient.h"
#include <string>
#include <vector>
#include <memory>
#include <commctrl.h>
#include <shellapi.h>

class FtpDockableDlg : public DockingDlgInterface
{
public:
    FtpDockableDlg();
    ~FtpDockableDlg();
    
    void init(HINSTANCE hInst, HWND parent);
    void setParent(HWND parent2set) {
        _hParent = parent2set;
    }
    
    void displayServerManager();
    void connectToServer(const std::string& profileId);
    void logMessage(const std::wstring& msg);

protected:
    virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
    
private:
    int getSystemIconIndex(const std::wstring& fileName, bool isDirectory);
    
    ServerManagerDlg _serverManager;
    HIMAGELIST _hImageList;
    
    struct ServerConnection {
        std::string profileId;
        std::string profileName;
        std::shared_ptr<FtpClient> client;
        HWND hTree;
    };
    
    std::vector<ServerConnection> _connections;
    int _activeConnectionIndex = -1;
    
    void switchToConnection(int index);
    void closeConnection(int index);
};

#endif // FTPDOCKABLEDLG_H
