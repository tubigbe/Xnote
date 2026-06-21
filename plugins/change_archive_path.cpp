#include "plugin_api.h"
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

static std::string GetConfigPath() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    fs::path p(buf);
    return (p.parent_path().parent_path() / "config.json").string();
}

static std::string ReadConfigValue(const std::string& key) {
    std::string cfgPath = GetConfigPath();
    if (!fs::exists(cfgPath)) return "";
    std::ifstream f(cfgPath);
    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();
    std::string search = "\"" + key + "\"";
    size_t pos = content.find(search);
    if (pos == std::string::npos) return "";
    pos = content.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < content.size() && content[pos] == ' ') pos++;
    if (pos >= content.size()) return "";
    if (content[pos] == '"') {
        size_t end = content.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return content.substr(pos + 1, end - pos - 1);
    }
    size_t end = pos;
    while (end < content.size() && content[end] != ',' && content[end] != '}' && content[end] != '\n') end++;
    return content.substr(pos, end - pos);
}

static void WriteConfigValue(const std::string& key, const std::string& value) {
    std::ofstream f(GetConfigPath());
    f << "{\n    \"" << key << "\": \"" << value << "\"\n}\n";
}

extern "C" {
    PLUGIN_API const char* GetHotkey() {
        return "alt+p";
    }

    PLUGIN_API void Run() {
        std::string currentPath = ReadConfigValue("archive_path");

        BROWSEINFOA bi = {};
        bi.lpszTitle = "Select Archive Folder";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        if (!currentPath.empty()) {
            bi.lpDisplayName = const_cast<char*>(currentPath.c_str());
        }

        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        if (!pidl) return;

        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            WriteConfigValue("archive_path", path);
            MessageBoxA(nullptr, ("Archive path updated to:\n" + std::string(path)).c_str(), "Xnote", MB_OK | MB_ICONINFORMATION);
        }

        CoTaskMemFree(pidl);
    }
}
