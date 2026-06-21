#pragma once

#include <windows.h>
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>

struct Plugin {
    std::string name;
    std::string hotkey;
    std::function<void()> run;
    HMODULE handle = nullptr;
};

class XnoteKernel {
public:
    XnoteKernel();
    ~XnoteKernel();

    bool Init();
    void Run();
    void Shutdown();

    static std::string GetConfigPath();
    static std::string ReadArchivePath();
    static bool WriteArchivePath(const std::string& path);

private:
    bool LoadPlugins();
    bool RegisterHotkey(const std::string& hotkey, const std::string& owner);
    void UnregisterAllHotkeys();
    void ShowError(const std::string& msg);
    void ShowInfo(const std::string& msg);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND hwnd_ = nullptr;
    NOTIFYICONDATA nid_ = {};
    std::vector<Plugin> plugins_;
    std::unordered_map<std::string, std::string> hotkeyRegistry_;
    std::unordered_map<int, std::string> hotkeyIdMap_;
    int nextHotkeyId_ = 1;
    static const UINT WM_HOTKEY_MSG = 0x0312;
    static const int TRAY_ICON_ID = 1;
    static const int MENU_OPEN_FOLDER = 1001;
    static const int MENU_VIEW_SHORTCUTS = 1002;
    static const int MENU_EXIT = 1003;
};
