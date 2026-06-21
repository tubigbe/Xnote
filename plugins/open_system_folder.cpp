#include "plugin_api.h"
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;

static std::string GetConfigPath() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    fs::path p(buf);
    return (p.parent_path().parent_path() / "config.json").string();
}

static std::string ReadArchivePath() {
    std::string cfgPath = GetConfigPath();
    if (!fs::exists(cfgPath)) return "";
    std::ifstream f(cfgPath);
    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();
    std::string search = "\"archive_path\"";
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

extern "C" {
    PLUGIN_API const char* GetHotkey() {
        return "alt+o";
    }

    PLUGIN_API void Run() {
        std::string basePath = ReadArchivePath();
        if (basePath.empty()) {
            MessageBoxA(nullptr, "Archive path not configured!", "Xnote Error", MB_OK | MB_ICONERROR);
            return;
        }

        time_t now = time(nullptr);
        struct tm tm_info;
        localtime_s(&tm_info, &now);

        char folderName[32];
        snprintf(folderName, sizeof(folderName), "%d.%02d",
            tm_info.tm_year + 1900, tm_info.tm_mon + 1);

        char fileName[32];
        snprintf(fileName, sizeof(fileName), "%d.%02d.%02d.txt",
            tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);

        fs::path targetPath = fs::path(basePath) / folderName;
        if (!fs::exists(targetPath)) {
            fs::create_directories(targetPath);
        }

        fs::path filePath = targetPath / fileName;
        if (!fs::exists(filePath)) {
            std::ofstream f(filePath);
        }

        ShellExecuteA(nullptr, "open", targetPath.string().c_str(), nullptr, nullptr, SW_SHOW);
        ShellExecuteA(nullptr, "open", filePath.string().c_str(), nullptr, nullptr, SW_SHOW);
    }
}
