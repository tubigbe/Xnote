#include "Xnote.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

namespace fs = std::filesystem;

// JSON helper (minimal, no external dependency)
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

XnoteKernel::XnoteKernel() {}

XnoteKernel::~XnoteKernel() {
    Shutdown();
}

std::string XnoteKernel::GetConfigPath() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    fs::path p(buf);
    return (p.parent_path() / "config.json").string();
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

void XnoteKernel::ShowInfo(const std::string& msg) {
    MessageBoxA(nullptr, msg.c_str(), "Xnote", MB_OK | MB_ICONINFORMATION);
}

bool XnoteKernel::Init() {
    // Create hidden window for message processing
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "XnoteMsgWindow";
    RegisterClassA(&wc);

    hwnd_ = CreateWindowExA(0, "XnoteMsgWindow", "Xnote", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleA(nullptr), this);
    if (!hwnd_) return false;

    SetWindowLongPtrA(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Create tray icon
    nid_.cbSize = sizeof(NOTIFYICONDATA);
    nid_.hWnd = hwnd_;
    nid_.uID = TRAY_ICON_ID;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_USER + 1;
    strcpy_s(nid_.szTip, "Xnote - Global Hotkey Notes");

    // Create a simple icon
    HICON hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    nid_.hIcon = hIcon;

    Shell_NotifyIconA(NIM_ADD, &nid_);

    // Try to open the archive folder on startup
    std::string archivePath = ReadArchivePath();
    if (!archivePath.empty() && fs::exists(archivePath)) {
        ShellExecuteA(nullptr, "open", archivePath.c_str(), nullptr, nullptr, SW_SHOW);
    }

    // Load plugins
    if (!LoadPlugins()) {
        ShowError("Failed to load plugins!");
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

        if (!RegisterHotkey(plugin.hotkey, plugin.name)) {
            FreeLibrary(plugin.handle);
            continue;
        }

        plugins_.push_back(plugin);
    }

    return true;
}

bool XnoteKernel::RegisterHotkey(const std::string& hotkey, const std::string& owner) {
    std::string normalized = hotkey;
    normalized.erase(std::remove(normalized.begin(), normalized.end(), ' '), normalized.end());
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

    if (hotkeyRegistry_.count(normalized)) {
        ShowError("Hotkey conflict: " + normalized + "\nUsed by: " + hotkeyRegistry_[normalized] + " and " + owner);
        return false;
    }

    UINT modifiers = 0;
    UINT vk = 0;
    std::istringstream iss(normalized);
    std::string token;

    while (std::getline(iss, token, '+')) {
        if (token == "alt") modifiers |= MOD_ALT;
        else if (token == "ctrl") modifiers |= MOD_CONTROL;
        else if (token == "shift") modifiers |= MOD_SHIFT;
        else if (token == "win") modifiers |= MOD_WIN;
        else if (token.length() == 1) {
            char c = toupper(token[0]);
            vk = c;
        }
    }

    if (!modifiers || !vk) return false;

    int id = nextHotkeyId_++;
    if (!RegisterHotKey(hwnd_, id, modifiers, vk)) return false;

    hotkeyRegistry_[normalized] = owner;
    hotkeyIdMap_[id] = normalized;
    return true;
}

void XnoteKernel::UnregisterAllHotkeys() {
    for (auto& [id, key] : hotkeyIdMap_) {
        UnregisterHotKey(hwnd_, id);
    }
    hotkeyIdMap_.clear();
    hotkeyRegistry_.clear();
}

LRESULT CALLBACK XnoteKernel::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    XnoteKernel* self = reinterpret_cast<XnoteKernel*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    if (msg == WM_USER + 1) {
        if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();
            AppendMenuA(menu, MF_STRING, MENU_OPEN_FOLDER, "Open Archive Folder");
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
    }

    if (msg == WM_HOTKEY) {
        int id = static_cast<int>(wp);
        if (self->hotkeyIdMap_.count(id)) {
            std::string key = self->hotkeyIdMap_[id];
            for (auto& plugin : self->plugins_) {
                std::string nk = plugin.hotkey;
                nk.erase(std::remove(nk.begin(), nk.end(), ' '), nk.end());
                std::transform(nk.begin(), nk.end(), nk.begin(), ::tolower);
                if (nk == key) {
                    plugin.run();
                    break;
                }
            }
        }
        return 0;
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
    UnregisterAllHotkeys();

    for (auto& plugin : plugins_) {
        if (plugin.handle) FreeLibrary(plugin.handle);
    }
    plugins_.clear();

    Shell_NotifyIconA(NIM_DELETE, &nid_);
}

int main() {
    XnoteKernel kernel;
    if (!kernel.Init()) return 1;
    kernel.Run();
    return 0;
}
