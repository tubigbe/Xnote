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
        fs::path projectDir = exePath.parent_path();

        if (fs::exists(projectDir)) {
            ShellExecuteA(nullptr, "open", projectDir.string().c_str(), nullptr, nullptr, SW_SHOW);
        }
    }
}
