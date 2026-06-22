#include "plugin_api.h"
#include <windows.h>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

extern "C" {
    PLUGIN_API const char* GetHotkey() {
        return "alt+shift+o";
    }

    PLUGIN_API void Run() {
        char buf[MAX_PATH];
        GetModuleFileNameA(nullptr, buf, MAX_PATH);
        fs::path exePath(buf);
        // EXE is in build/, plugins are in build/plugins/, project root is one up
        fs::path projectDir = exePath.parent_path().parent_path();

        if (fs::exists(projectDir)) {
            ShellExecuteA(nullptr, "open", "explorer.exe", projectDir.string().c_str(), nullptr, SW_SHOW);
        }
    }
}
