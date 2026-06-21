#include "Xnote.h"
#include "resource.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

namespace fs = std::filesystem;

// Create a red apple tray icon
HICON CreateAppleIcon() {
    const int size = 64;
    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, size, size);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

    // Fill with transparent
    HBRUSH hBrushBg = CreateSolidBrush(RGB(192, 192, 192));
    RECT rc = {0, 0, size, size};
    FillRect(memDC, &rc, hBrushBg);
    DeleteObject(hBrushBg);

    // Apple body (red)
    HBRUSH hBrushRed = CreateSolidBrush(RGB(220, 30, 30));
    HPEN hPenRed = CreatePen(PS_SOLID, 1, RGB(180, 20, 20));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(memDC, hBrushRed);
    HPEN hOldPen = (HPEN)SelectObject(memDC, hPenRed);
    Ellipse(memDC, 12, 18, 52, 58);
    SelectObject(memDC, hOldBrush);
    SelectObject(memDC, hOldPen);
    DeleteObject(hBrushRed);
    DeleteObject(hPenRed);

    // Small indent at top
    HBRUSH hBrushBg2 = CreateSolidBrush(RGB(192, 192, 192));
    SelectObject(memDC, hBrushBg2);
    Ellipse(memDC, 26, 14, 38, 24);
    SelectObject(memDC, hOldBrush);
    DeleteObject(hBrushBg2);

    // Stem (brown)
    HPEN hPenBrown = CreatePen(PS_SOLID, 2, RGB(100, 60, 20));
    SelectObject(memDC, hPenBrown);
    MoveToEx(memDC, 32, 16, nullptr);
    LineTo(memDC, 34, 8);
    SelectObject(memDC, hOldPen);
    DeleteObject(hPenBrown);

    // Leaf (green)
    HBRUSH hBrushGreen = CreateSolidBrush(RGB(40, 180, 40));
    HPEN hPenGreen = CreatePen(PS_SOLID, 1, RGB(20, 120, 20));
    SelectObject(memDC, hBrushGreen);
    SelectObject(memDC, hPenGreen);
    POINT leaf[] = {{34, 10}, {44, 6}, {38, 14}};
    Polygon(memDC, leaf, 3);
    SelectObject(memDC, hOldBrush);
    SelectObject(memDC, hOldPen);
    DeleteObject(hBrushGreen);
    DeleteObject(hPenGreen);

    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = hBitmap;
    iconInfo.hbmColor = hBitmap;
    HICON hIcon = CreateIconIndirect(&iconInfo);

    SelectObject(memDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);

    return hIcon;
}

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

    // Load red apple icon from embedded resource
    nid_.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON));

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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    XnoteKernel kernel;
    if (!kernel.Init()) return 1;
    kernel.Run();
    return 0;
}
