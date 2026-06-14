//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include "DockingFeature/FtpDockableDlg.h"
#include <algorithm>
#include <string>
#include <stdio.h>

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

// Global instance of our dockable dialog
FtpDockableDlg ftpDockDlg;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    ftpDockDlg.init((HINSTANCE)hModule, NULL);
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
    ftpDockDlg.destroy();
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Panel FTP/SFTP"), showFtpPanel, NULL, false);
    setCommand(1, TEXT("Menedżer Serwerów"), showServerManager, NULL, false);
    setCommand(2, TEXT("Sprawdź aktualizacje..."), checkForUpdates, NULL, false);
    setCommand(3, TEXT("O wtyczce PandaFTP"), showAbout, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void showFtpPanel()
{
    if (!ftpDockDlg.isCreated()) {
        ftpDockDlg.setParent(nppData._nppHandle);
        
        tTbData data;
        ftpDockDlg.create(&data);
        
        data.pszName = (LPWSTR)L"PandaFTP";
        data.dlgID = 0; // Command index corresponding to "showFtpPanel"
        data.uMask = DWS_DF_CONT_RIGHT | DWS_ICONTAB | DWS_ICONBAR;
        data.hIconTab = (HICON)::LoadImage(NULL, MAKEINTRESOURCE(32512), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        data.pszModuleName = (LPWSTR)NPP_PLUGIN_NAME;
        
        ::SendMessage(nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
        
        // Let DMM handle showing
        // ::SendMessage(nppData._nppHandle, NPPM_DMMSHOW, 0, (LPARAM)ftpDockDlg.getHSelf());
    } else {
        // Toggle visibility via DMM (Docking Manager)
        if (ftpDockDlg.isVisible()) {
            ::SendMessage(nppData._nppHandle, NPPM_DMMHIDE, 0, (LPARAM)ftpDockDlg.getHSelf());
        } else {
            ::SendMessage(nppData._nppHandle, NPPM_DMMSHOW, 0, (LPARAM)ftpDockDlg.getHSelf());
        }
    }
}

void showServerManager()
{
    ftpDockDlg.displayServerManager();
}

void checkForUpdates()
{
    ::ShellExecuteW(NULL, L"open", L"https://github.com/deser1/PandaFTP/releases", NULL, NULL, SW_SHOWNORMAL);
}

void showAbout()
{
    wchar_t langPath[MAX_PATH] = {0};
    ::SendMessage(nppData._nppHandle, NPPM_GETNATIVELANGFILENAME, MAX_PATH, (LPARAM)langPath);
    
    std::wstring aboutText = 
        L"PandaFTP v1.0.0\n\n"
        L"Wtyczka dla Notepad++ do obsługi serwerów FTP i SFTP.\n"
        L"Autor: Domek Software\n\n"
        L"Ścieżka języka N++: " + std::wstring(langPath) + L"\n\n"
        L"Więcej informacji znajdziesz na GitHubie.";
        
    ::MessageBoxW(nppData._nppHandle, aboutText.c_str(), L"O wtyczce PandaFTP", MB_OK | MB_ICONINFORMATION);
}

bool isNotepadLanguagePolish()
{
    // Method 1: Check the Notepad++ main menu text (100% reliable for current UI language)
    HMENU hMenu = ::GetMenu(nppData._nppHandle);
    if (hMenu) {
        wchar_t menuText[256] = {0};
        
        // First menu item: "File" -> "Plik"
        ::GetMenuStringW(hMenu, 0, menuText, 255, MF_BYPOSITION);
        std::wstring wMenu0(menuText);
        std::transform(wMenu0.begin(), wMenu0.end(), wMenu0.begin(), [](wchar_t c) { return (wchar_t)::towlower(c); });
        
        // Second menu item: "Edit" -> "Edycja"
        ::GetMenuStringW(hMenu, 1, menuText, 255, MF_BYPOSITION);
        std::wstring wMenu1(menuText);
        std::transform(wMenu1.begin(), wMenu1.end(), wMenu1.begin(), [](wchar_t c) { return (wchar_t)::towlower(c); });
        
        if (wMenu0.find(L"plik") != std::wstring::npos || wMenu1.find(L"edycja") != std::wstring::npos) {
            return true;
        }
        
        // If we successfully read the menu and it's NOT Polish, return false immediately
        if (!wMenu0.empty() || !wMenu1.empty()) {
            return false;
        }
    }

    // Method 2: Fallback to checking the localization file
    wchar_t langPath[MAX_PATH] = {0};
    ::SendMessage(nppData._nppHandle, NPPM_GETNATIVELANGFILENAME, MAX_PATH, (LPARAM)langPath);
    std::wstring wLangPath(langPath);
    
    // Fallback: If empty, assume Polish as default for this plugin
    if (wLangPath.empty()) {
        return true;
    }
    
    std::wstring wLangLower = wLangPath;
    std::transform(wLangLower.begin(), wLangLower.end(), wLangLower.begin(), [](wchar_t c) { return (wchar_t)::towlower(c); });
    
    if (wLangLower.find(L"polish") != std::wstring::npos || wLangLower.find(L"polski") != std::wstring::npos) {
        return true;
    }
    
    // If it's a generic nativeLang.xml, read the file to find out
    if (wLangLower.find(L"nativelang.xml") != std::wstring::npos) {
        FILE* f = nullptr;
        _wfopen_s(&f, langPath, L"r");
        if (f) {
            char buf[1024] = {0};
            fread(buf, 1, 1023, f);
            fclose(f);
            std::string content(buf);
            std::transform(content.begin(), content.end(), content.begin(), [](char c) { return (char)::tolower((unsigned char)c); });
            if (content.find("polish") != std::string::npos || content.find("polski") != std::string::npos) {
                return true;
            }
        }
    }
    
    // Check Windows locale as a last resort
    LANGID langid = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(langid) == LANG_POLISH) {
        return true;
    }
    
    return false;
}
