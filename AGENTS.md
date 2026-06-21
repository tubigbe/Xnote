# AGENTS.md

## Project Overview

Xnote is a Windows desktop global-hotkey app (C++). Microkernel + plugin architecture: `Xnote.cpp` is the kernel, `plugins/*.cpp` are hotkey extension DLLs.

## Build

- **Prerequisites**: Visual Studio with C++ workload (needs `cl.exe`)
- **Build command**: Run from VS Developer Command Prompt: `build.bat`
- **Output**: `build/Xnote.exe` + `build/plugins/*.dll`
- **No external dependencies**: Uses only Windows API (user32, shell32, ole32)

## Runtime

- **Platform**: Windows only (uses `RegisterHotKey`, system tray, `ShellExecute`)
- **Entry point**: `build\Xnote.exe`
- **Config**: `config.json` at exe directory, stores `{"archive_path": "..."}`. Plugins read/write this directly (not through the kernel).

## Plugin Contract

Each plugin is a DLL in `plugins/` that exports two C functions:
- `GetHotkey()` → returns hotkey string, e.g. `"alt+t"`, `"alt+shift+o"`
- `Run()` → zero-arg function, invoked when the hotkey fires

Include `plugins/plugin_api.h` for the correct declarations. The kernel auto-discovers `*.dll` files via `LoadLibrary`. Hotkey conflicts trigger a fatal error dialog.

## Architecture

```
Xnote.h          — Kernel class declaration
Xnote.cpp        — Kernel: tray icon, hotkey dispatch, plugin loader
plugins/
  plugin_api.h   — DLL export interface
  change_archive_path.dll  — Alt+P: change archive folder
  inject_timestamp.dll     — Alt+T: type timestamp at cursor
  open_plugins_folder.dll  — Alt+Shift+O: open project dir
  open_system_folder.dll   — Alt+O: open today's note file
```

## Conventions

- Hotkey strings are normalized to lowercase with no spaces (e.g. `"alt+shift+o"`).
- Plugins access shared state (archive path) by reading/writing `config.json` directly — not via kernel globals.
- The app runs as a system tray icon with no console window. Exit via tray right-click.
- All UI messages are in Chinese.
- Use `/EHsc` and `/utf-8` flags for C++ compilation.
- Plugins link against `user32.lib` at minimum; `shell32.lib` for folder operations.
