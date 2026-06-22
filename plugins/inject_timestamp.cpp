#include "plugin_api.h"
#include <windows.h>
#include <cstdio>
#include <ctime>

static void TypeChar(char c) {
    INPUT inputs[4] = {};
    int count = 0;

    SHORT vk = VkKeyScanA(c);
    bool needShift = (vk >> 8) & 1;
    BYTE scanCode = (vk >> 8) >> 8;

    if (needShift) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_SHIFT;
        inputs[count].ki.dwFlags = 0;
        count++;
    }

    inputs[count].type = INPUT_KEYBOARD;
    inputs[count].ki.wVk = vk & 0xFF;
    inputs[count].ki.wScan = scanCode;
    inputs[count].ki.dwFlags = 0;
    count++;

    inputs[count].type = INPUT_KEYBOARD;
    inputs[count].ki.wVk = vk & 0xFF;
    inputs[count].ki.wScan = scanCode;
    inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
    count++;

    if (needShift) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_SHIFT;
        inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
        count++;
    }

    SendInput(count, inputs, sizeof(INPUT));
    Sleep(10);
}

static void TypeEnter() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_RETURN;
    inputs[0].ki.dwFlags = 0;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_RETURN;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
    Sleep(10);
}

extern "C" {
    PLUGIN_API const char* GetHotkey() {
        return "alt+y";
    }

    PLUGIN_API void Run() {
        time_t now = time(nullptr);
        struct tm ti;
        localtime_s(&ti, &now);

        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d %d/%d/%d",
            ti.tm_hour, ti.tm_min,
            ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday);

        for (int i = 0; buf[i]; i++) {
            TypeChar(buf[i]);
        }
        TypeEnter();
        TypeEnter();
    }
}
