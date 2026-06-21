# Xnote

A Windows global hotkey app for quick note-taking. Microkernel + plugin architecture — the core is minimal, all features come from hotkey plugins.

## Features

- **System tray** — runs silently in the notification area, no taskbar window
- **Global hotkeys** — works from any app
- **Plugin system** — drop a DLL into `plugins/` to add new hotkeys
- **Hotkey conflict detection** — warns immediately if two plugins claim the same key

### Built-in plugins

| Hotkey | Plugin | Action |
|--------|--------|--------|
| Alt+P | change_archive_path | Change the note archive folder |
| Alt+T | inject_timestamp | Type current timestamp at cursor |
| Alt+O | open_system_folder | Open today's note file |
| Alt+Shift+O | open_plugins_folder | Open the project folder |
| Right-click → View Shortcuts | — | Show all registered hotkeys |

## Requirements

- Windows 10/11
- [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) with C++ workload (for `cl.exe`)

## Build

Open a **Developer Command Prompt** and run:

```
build.bat
```

Output: `build/Xnote.exe` + `build/plugins/*.dll`

## Run

```
build\Xnote.exe
```

On first run, select your note archive folder. The app creates `config.json` to remember it.

## Writing a plugin

Create a `.cpp` file in `plugins/` that exports two C functions:

```cpp
#include "plugin_api.h"

extern "C" {
    PLUGIN_API const char* GetHotkey() {
        return "alt+f1";
    }

    PLUGIN_API void Run() {
        // your logic here
    }
}
```

Compile as a DLL and drop it into `build/plugins/`. The kernel loads it automatically on next launch.

## License

MIT
