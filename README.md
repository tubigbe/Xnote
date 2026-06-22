# Notes

A Windows global hotkey app for quick note-taking. Microkernel + plugin architecture — the core is minimal, all features come from hotkey plugins.

## Features

- **System tray** — runs silently in the notification area, no taskbar window
- **Global hotkeys** — works from any app
- **Plugin system** — drop a `.py` file into `plugins/` to add new hotkeys
- **Hotkey conflict detection** — warns immediately if two plugins claim the same key

### Built-in plugins

| Hotkey | Plugin | Action |
|--------|--------|--------|
| Alt+P | change_archive_path | Change the note archive folder |
| Alt+T | inject_timestamp | Type current timestamp at cursor |
| Alt+O | open_system_folder | Open today's note file |
| Alt+Shift+O | open_plugins_folder | Open the project folder |

## Requirements

- Windows 10/11
- Python 3.8+
- `pip install keyboard pystray Pillow`

## Run

```
python notes.py
```

On first run, select your note archive folder. The app creates `config.json` to remember it.

## Writing a plugin

Create a `.py` file in `plugins/` with two exports:

```python
HOTKEY = "alt+f1"

def run():
    # your logic here
    pass
```

The kernel loads it automatically on next launch.

## License

MIT
