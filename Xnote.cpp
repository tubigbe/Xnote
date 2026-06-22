#include "Xnote.h"
#include "resource.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

namespace fs = std::filesystem;

static XnoteKernel* g_instance = nullptr;

namespace json {
    std::string ReadFile(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return "";
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    std::string GetValue(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < json.size() && json[pos] == ' ') pos++;
        if (pos >= json.size()) return "";
        if (json[pos] == '"') {
            size_t end = json.find('"', pos + 1);
            if (end == std::string::npos) return "";
            return json.substr(pos + 1, end - pos - 1);
        }
        size_t end = pos;
        while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != '\n') end++;
        return json.substr(pos, end - pos);
    }

    void WriteFile(const std::string& path, const std::string& key, const std::string& value) {
        std::ofstream f(path);
        f << "{\n    \"" << key << "\": \"" << value << "\"\n}\n";
    }
}

XnoteKernel::XnoteKernel() {
    g_instance = this;
}

XnoteKernel::~XnoteKernel() {
    Shutdown();
}

std::string XnoteKernel::GetConfigPath() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    fs::path p(buf);
    return (p.parent_path().parent_path() / "config.json").string();
}

std::string XnoteKernel::ReadArchivePath() {
    std::string cfg = GetConfigPath();
    if (!fs::exists(cfg)) return "";
    std::string content = json::ReadFile(cfg);
    return json::GetValue(content, "archive_path");
}

bool XnoteKernel::WriteArchivePath(const std::string& path) {
    json::WriteFile(GetConfigPath(), "archive_path", path);
    return true;
}

void XnoteKernel::ShowError(const std::string& msg) {
    MessageBoxA(nullptr, msg.c_str(), "Xnote Error", MB_OK | MB_ICONERROR);
}

bool XnoteKernel::ParseHotkey(const std::string& hotkey, UINT& outMod, UINT& outVk) {
    std::string s = hotkey;
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    outMod = 0;
    outVk = 0;
    std::istringstream iss(s);
    std::string token;

    while (std::getline(iss, token, '+')) {
        if (token == "alt") outMod |= MOD_ALT;
        else if (token == "ctrl") outMod |= MOD_CONTROL;
        else if (token == "shift") outMod |= MOD_SHIFT;
        else if (token == "win") outMod |= MOD_WIN;
        else if (token.length() == 1) outVk = toupper(token[0]);
    }

    return (outMod != 0 && outVk != 0);
}

bool XnoteKernel::Init() {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "XnoteMsgWindow";
    RegisterClassA(&wc);

    hwnd_ = CreateWindowExA(0, "XnoteMsgWindow", "Xnote", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleA(nullptr), this);
    if (!hwnd_) return false;

    SetWindowLongPtrA(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    nid_.cbSize = sizeof(NOTIFYICONDATA);
    nid_.hWnd = hwnd_;
    nid_.uID = TRAY_ICON_ID;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_USER + 1;
    strcpy_s(nid_.szTip, "Xnote - Global Hotkey Notes");
    nid_.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON));
    Shell_NotifyIconA(NIM_ADD, &nid_);

    std::string archivePath = ReadArchivePath();
    if (!archivePath.empty() && fs::exists(archivePath)) {
        ShellExecuteA(nullptr, "open", archivePath.c_str(), nullptr, nullptr, SW_SHOW);
    }

    if (!LoadPlugins()) {
        ShowError("Failed to load plugins!");
        return false;
    }

    keyboardHook_ = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleA(nullptr), 0);
    if (!keyboardHook_) {
        ShowError("Failed to install keyboard hook!");
        return false;
    }

    return true;
}

bool XnoteKernel::LoadPlugins() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    fs::path pluginDir = fs::path(buf).parent_path() / "plugins";

    if (!fs::exists(pluginDir)) {
        fs::create_directories(pluginDir);
        return true;
    }

    for (auto& entry : fs::directory_iterator(pluginDir)) {
        if (entry.path().extension() != ".dll") continue;

        Plugin plugin;
        plugin.name = entry.path().stem().string();
        plugin.handle = LoadLibraryA(entry.path().string().c_str());
        if (!plugin.handle) continue;

        using GetHotkeyFn = const char* (*)();
        using RunFn = void (*)();

        auto getHotkey = reinterpret_cast<GetHotkeyFn>(GetProcAddress(plugin.handle, "GetHotkey"));
        auto run = reinterpret_cast<RunFn>(GetProcAddress(plugin.handle, "Run"));

        if (!getHotkey || !run) {
            FreeLibrary(plugin.handle);
            continue;
        }

        plugin.hotkey = getHotkey();
        plugin.run = run;

        UINT mod = 0, vk = 0;
        if (!ParseHotkey(plugin.hotkey, mod, vk)) {
            FreeLibrary(plugin.handle);
            continue;
        }

        // Check for conflicts
        for (auto& h : hotkeys_) {
            if (h.modifier == mod && h.vk == vk) {
                ShowError("Hotkey conflict: " + plugin.hotkey + "\nUsed by: " + h.owner + " and " + plugin.name);
                FreeLibrary(plugin.handle);
                goto next;
            }
        }

        hotkeys_.push_back({mod, vk, plugin.name, plugin.run});
        plugins_.push_back(plugin);
        next:;
    }

    return true;
}

LRESULT CALLBACK XnoteKernel::KeyboardProc(int nCode, WPARAM wp, LPARAM lp) {
    if (nCode >= 0 && g_instance) {
        KBDLLHOOKSTRUCT* ks = reinterpret_cast<KBDLLHOOKSTRUCT*>(lp);

        // Track modifier state
        if (ks->vkCode == VK_LMENU || ks->vkCode == VK_RMENU)
            g_instance->altHeld_ = (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN);
        if (ks->vkCode == VK_LCONTROL || ks->vkCode == VK_RCONTROL)
            g_instance->ctrlHeld_ = (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN);
        if (ks->vkCode == VK_LSHIFT || ks->vkCode == VK_RSHIFT)
            g_instance->shiftHeld_ = (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN);
        if (ks->vkCode == VK_LWIN || ks->vkCode == VK_RWIN)
            g_instance->winHeld_ = (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN);

        // Only trigger on key down (not repeat)
        if (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN) {
            UINT currentMod = 0;
            if (g_instance->altHeld_) currentMod |= MOD_ALT;
            if (g_instance->ctrlHeld_) currentMod |= MOD_CONTROL;
            if (g_instance->shiftHeld_) currentMod |= MOD_SHIFT;
            if (g_instance->winHeld_) currentMod |= MOD_WIN;

            for (auto& h : g_instance->hotkeys_) {
                if (h.modifier == currentMod && h.vk == ks->vkCode) {
                    // Store and post to main thread
                    g_instance->pendingRun_ = h.run;
                    PostMessage(g_instance->hwnd_, WM_APP + 1, 0, 0);
                    return 1; // Suppress the keystroke
                }
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wp, lp);
}

LRESULT CALLBACK XnoteKernel::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    XnoteKernel* self = reinterpret_cast<XnoteKernel*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    if (msg == WM_APP + 1) {
        if (self->pendingRun_) {
            self->pendingRun_();
            self->pendingRun_ = nullptr;
        }
        return 0;
    }

    if (msg == WM_USER + 1) {
        if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();
            AppendMenuA(menu, MF_STRING, MENU_OPEN_FOLDER, "Open Archive Folder");
            AppendMenuA(menu, MF_STRING, MENU_VIEW_SHORTCUTS, "View Shortcuts");
            AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuA(menu, MF_STRING, MENU_EXIT, "Exit Xnote");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
        }
        return 0;
    }

    if (msg == WM_COMMAND) {
        if (LOWORD(wp) == MENU_EXIT) {
            self->Shutdown();
            PostQuitMessage(0);
            return 0;
        }
        if (LOWORD(wp) == MENU_OPEN_FOLDER) {
            std::string path = ReadArchivePath();
            if (!path.empty() && fs::exists(path)) {
                ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOW);
            }
            return 0;
        }
        if (LOWORD(wp) == MENU_VIEW_SHORTCUTS) {
            std::string msg = "Current Shortcuts:\n\n";
            for (const auto& plugin : self->plugins_) {
                msg += plugin.hotkey + "  ->  " + plugin.name + "\n";
            }
            if (self->plugins_.empty()) {
                msg += "(no plugins loaded)";
            }
            MessageBoxA(nullptr, msg.c_str(), "Xnote Shortcuts", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
    }

    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

void XnoteKernel::Run() {
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void XnoteKernel::Shutdown() {
    if (keyboardHook_) {
        UnhookWindowsHookEx(keyboardHook_);
        keyboardHook_ = nullptr;
    }

    for (auto& plugin : plugins_) {
        if (plugin.handle) FreeLibrary(plugin.handle);
    }
    plugins_.clear();
    hotkeys_.clear();

    Shell_NotifyIconA(NIM_DELETE, &nid_);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    XnoteKernel kernel;
    if (!kernel.Init()) return 1;
    kernel.Run();
    return 0;
}
