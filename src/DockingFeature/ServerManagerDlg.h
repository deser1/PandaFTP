#ifndef SERVERMANAGERDLG_H
#define SERVERMANAGERDLG_H

#include "StaticDialog.h"
#include "resource.h"
#include "../ConfigManager.h"

class ServerManagerDlg : public StaticDialog
{
public:
    ServerManagerDlg() : StaticDialog() {}
    
    void doDialog() {
        if (!isCreated()) {
            create(IDD_SERVER_MGR);
        }
        goToCenter();
        display();
    }

protected:
    virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
    void refreshProfileList();
    void loadProfileData(const std::string& id);
    bool saveCurrentProfile();
    void clearForm();
    
    std::string currentProfileId;
};

#endif // SERVERMANAGERDLG_H
