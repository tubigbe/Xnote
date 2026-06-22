# Notes - Architecture & Design Document

This project uses a Microkernel & Plugin decoupled architecture. The main program acts as a logic-free control core, dynamically dispatching external plugin functions through standard contract interfaces, enabling system-wide hotkey efficiency extensions.

## I. Core Kernel Functions

The main program (`notes.py`) carries no hotkey business logic. Its core responsibilities are:

Environment & Memory Awareness: On startup, automatically reads the local `config.json`. On first run or invalid path, guides the user to set up the notes archive folder via a lightweight GUI.

Native System Dispatch: After config is set, the kernel invokes the Windows API to open the native file explorer at the target directory, delegating all file management to the OS.

Dynamic Multi-Plugin Loading Engine: The kernel scans and loads all `.py` plugin files in the `plugins/` directory at runtime via `glob` and `importlib`, enabling hot-pluggable features without recompilation.

Global Hotkey Collision Interceptor: The kernel maintains a unified hotkey registry. If any plugin tries to register an already-taken hotkey, the interceptor immediately aborts loading and shows a Windows error dialog with the conflicting file names.

Stealth Guardian & Lifecycle Management: The program runs as a system tray icon in the Windows notification area with no taskbar window or console. On tray right-click exit, the kernel safely releases all system-level keyboard hooks.

## II. Plugin Contract

Plugins do not communicate with the kernel via global variables. Instead, they use dynamic reflection and a standard interface contract.

### Plugin Side

Any file placed in `plugins/` must expose exactly two standard interfaces:

- `HOTKEY` (string variable): Declares the global hotkey combination this plugin wants (e.g. `"alt+t"`).
- `run()` (zero-arg function): Contains the core logic executed when the hotkey fires.

### Kernel Side

The kernel drives plugin loading via this flow:

1. Scan plugins/*.py
2. Read file bytecode
3. Extract HOTKEY and run from each module
4. Run conflict check
5. If no conflict -> register in memory registry -> attach to keyboard listener (keyboard library)

### Data Isolation

All plugins have independent runtime contexts. If a plugin needs to access or change the archive path, it reads/writes `config.json` directly, bypassing the kernel. This ensures forward compatibility even as the kernel evolves.
