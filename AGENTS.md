# AGENTS.md

## Project Overview

Notes is a Windows desktop global-hotkey app (Python). Microkernel + plugin architecture: `notes.py` is the kernel, `plugins/*.py` are hotkey extensions.

## Runtime

- **Platform**: Windows only (uses `os.startfile`, system tray, `keyboard` global hooks)
- **Python deps**: `keyboard`, `pystray`, `Pillow` — no requirements.txt; install manually: `pip install keyboard pystray Pillow`
- **Entry point**: `python notes.py`
- **Config**: `config.json` at project root, stores `{"archive_path": "..."}`. Plugins read/write this directly (not through the kernel).

## Plugin Contract

Each file in `plugins/` must expose exactly two things:
- `HOTKEY` — string, e.g. `"alt+t"`, `"alt+shift+o"`
- `run()` — zero-arg callable, invoked when the hotkey fires

The kernel auto-discovers plugins via `glob` + `importlib`. No registration step needed. Hotkey conflicts trigger a fatal error dialog; the kernel keeps an in-memory registry to detect duplicates.

## Conventions

- Hotkey strings are normalized to lowercase with no spaces (e.g. `"alt+shift+o"`).
- Plugins access shared state (archive path) by reading/writing `config.json` directly — not via kernel globals.
- The app runs as a system tray icon with no console window. Exit via tray right-click.
- All UI messages are in English.
