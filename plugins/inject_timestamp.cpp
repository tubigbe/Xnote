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

        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "%02d:%02d %d/%d/%d\n\n",
            tm_info.tm_hour, tm_info.tm_min,
            tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);

        // Simulate typing the timestamp
        size_t len = strlen(timestamp);
        INPUT inputs[256];
        int count = 0;

        for (size_t i = 0; i < len && count < 250; i++) {
            char c = timestamp[i];
            if (c == '\n') {
                inputs[count].type = INPUT_KEYBOARD;
                inputs[count].ki.wVk = VK_RETURN;
                inputs[count].ki.dwFlags = 0;
                count++;
                inputs[count].type = INPUT_KEYBOARD;
                inputs[count].ki.wVk = VK_RETURN;
                inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
                count++;
            } else {
                SHORT vk = VkKeyScanA(c);
                if (vk != -1) {
                    if (vk & 0x100) { // Shift needed
                        inputs[count].type = INPUT_KEYBOARD;
                        inputs[count].ki.wVk = VK_SHIFT;
                        inputs[count].ki.dwFlags = 0;
                        count++;
                    }
                    inputs[count].type = INPUT_KEYBOARD;
                    inputs[count].ki.wVk = vk & 0xFF;
                    inputs[count].ki.dwFlags = 0;
                    count++;
                    inputs[count].type = INPUT_KEYBOARD;
                    inputs[count].ki.wVk = vk & 0xFF;
                    inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
                    count++;
                    if (vk & 0x100) {
                        inputs[count].type = INPUT_KEYBOARD;
                        inputs[count].ki.wVk = VK_SHIFT;
                        inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
                        count++;
                    }
                }
            }
        }

        if (count > 0) {
            SendInput(count, inputs, sizeof(INPUT));
        }
    }
}
