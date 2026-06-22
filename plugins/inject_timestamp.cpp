#include "plugin_api.h"
#include <windows.h>
#include <string>
#include <ctime>

extern "C" {
    PLUGIN_API const char* GetHotkey() {
        return "alt+t";
    }

    PLUGIN_API void Run() {
        time_t now = time(nullptr);
    struct tm tm_info;
    localtime_s(&tm_info, &now);

    wchar_t timestamp[64];
    swprintf(timestamp, 64, L"%02d:%02d %d/%d/%d\r\n\r\n",
        tm_info.tm_hour, tm_info.tm_min,
        tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);

    size_t len = wcslen(timestamp);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
    if (!hMem) return;

    wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
    memcpy(pMem, timestamp, (len + 1) * sizeof(wchar_t));
    GlobalUnlock(hMem);

    if (!OpenClipboard(nullptr)) {
        GlobalFree(hMem);
        return;
    }
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();

    Sleep(100);

    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
    }
}
