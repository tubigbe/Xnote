import os
import json
import glob
import importlib.util
import tkinter as tk
from tkinter import filedialog, messagebox

import keyboard
import pystray
from pystray import MenuItem as item
from PIL import Image, ImageDraw

CONFIG_FILE = "config.json"
PLUGIN_DIR = "plugins"
HOTKEY_REGISTRY = {}


def get_or_create_config():
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                config = json.load(f)
                saved_path = config.get("archive_path")
                if saved_path and os.path.exists(saved_path):
                    return saved_path
        except Exception as e:
            print(f"Failed to read config: {e}")

    root = tk.Tk()
    root.withdraw()
    messagebox.showinfo("Setup", "Please select or create your notes archive folder")
    selected_path = filedialog.askdirectory(title="Select your notes archive folder")

    if not selected_path:
        messagebox.showerror("Error", "An archive path is required to run the program!")
        root.destroy()
        exit()

    try:
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            json.dump({"archive_path": selected_path}, f, ensure_ascii=False, indent=4)
    except Exception as e:
        messagebox.showerror("Error", f"Cannot write config file: {e}")
        root.destroy()
        exit()

    root.destroy()
    return selected_path


def tray_open_folder(icon=None, item=None):
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                path = json.load(f).get("archive_path")
                if path and os.path.exists(path):
                    os.startfile(path)
                    return
        except:
            pass
    messagebox.showerror("Error", "Cannot open folder — path is invalid or config is missing!")


def register_hotkey(hotkey_str, callback_func, owner_name):
    normalized_hotkey = hotkey_str.strip().lower().replace(" ", "")

    if normalized_hotkey in HOTKEY_REGISTRY:
        conflicting_owner = HOTKEY_REGISTRY[normalized_hotkey]
        root = tk.Tk()
        root.withdraw()
        messagebox.showerror(
            "Hotkey Conflict",
            f"Hotkey collision detected, aborting load!\n\n"
            f"Conflicting hotkey: {hotkey_str}\n"
            f"Plugin 1: {conflicting_owner}\n"
            f"Plugin 2: {owner_name}\n\n"
            f"Please check your plugin files or change the conflicting hotkey."
        )
        root.destroy()
        return False

    HOTKEY_REGISTRY[normalized_hotkey] = owner_name
    keyboard.add_hotkey(normalized_hotkey, callback_func)
    return True


def load_dynamic_plugins():
    if not os.path.exists(PLUGIN_DIR):
        os.makedirs(PLUGIN_DIR)
        return

    plugin_files = glob.glob(os.path.join(PLUGIN_DIR, "*.py"))

    for file_path in plugin_files:
        file_name = os.path.basename(file_path)
        if file_name == "__init__.py":
            continue

        try:
            spec = importlib.util.spec_from_file_location(file_name[:-3], file_path)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)

            if hasattr(module, "HOTKEY") and hasattr(module, "run"):
                plugin_hotkey = getattr(module, "HOTKEY")
                register_hotkey(plugin_hotkey, module.run, f"plugin -> {file_name}")
        except Exception as e:
            print(f"Error loading plugin {file_name}: {e}")


def create_apple_icon():
    img = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    draw.ellipse([12, 18, 52, 58], fill=(220, 30, 30), outline=(180, 20, 20))

    draw.ellipse([26, 14, 38, 24], fill=(0, 0, 0, 0))

    draw.line([(32, 16), (34, 8)], fill=(100, 60, 20), width=2)

    draw.polygon([(34, 10), (44, 6), (38, 14)], fill=(40, 180, 40))

    return img


def exit_program(icon, item):
    keyboard.remove_all_hotkeys()
    icon.stop()


if __name__ == "__main__":
    initial_path = get_or_create_config()

    try:
        os.startfile(initial_path)
    except:
        pass

    load_dynamic_plugins()

    tray_menu = pystray.Menu(
        item('Open Notes Folder', tray_open_folder),
        item('Exit Notes', exit_program)
    )
    icon = pystray.Icon("notes", create_apple_icon(), "Notes - Active", menu=tray_menu)
    icon.run()
