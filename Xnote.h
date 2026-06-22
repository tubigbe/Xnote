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

struct HotkeyDef {
    UINT modifier;      // MOD_ALT, MOD_CONTROL, MOD_SHIFT
    UINT vk;            // virtual key code
    std::string owner;
    std::function<void()> run;
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
    bool ParseHotkey(const std::string& hotkey, UINT& outMod, UINT& outVk);
    void ShowError(const std::string& msg);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wp, LPARAM lp);

    HWND hwnd_ = nullptr;
    NOTIFYICONDATA nid_ = {};
    HHOOK keyboardHook_ = nullptr;
    std::vector<Plugin> plugins_;
    std::vector<HotkeyDef> hotkeys_;
    std::function<void()> pendingRun_;
    bool altHeld_ = false;
    bool ctrlHeld_ = false;
    bool shiftHeld_ = false;
    bool winHeld_ = false;

    static const int TRAY_ICON_ID = 1;
    static const int MENU_OPEN_FOLDER = 1001;
    static const int MENU_VIEW_SHORTCUTS = 1002;
    static const int MENU_EXIT = 1003;
};
