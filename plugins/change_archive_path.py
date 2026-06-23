import os
import sys
import json
import tkinter as tk
from tkinter import filedialog, messagebox

if getattr(sys, 'frozen', False):
    BASE_DIR = os.path.dirname(sys.executable)
else:
    BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

HOTKEY = "alt+p"
CONFIG_FILE = os.path.join(BASE_DIR, "config.json")


def run():
    root = tk.Tk()
    root.withdraw()

    current_path = ""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                current_path = json.load(f).get("archive_path", "")
        except:
            pass

    new_path = filedialog.askdirectory(title="Select your notes archive folder", initialdir=current_path)
    if new_path:
        try:
            with open(CONFIG_FILE, "w", encoding="utf-8") as f:
                json.dump({"archive_path": new_path}, f, ensure_ascii=False, indent=4)
            messagebox.showinfo("Success", f"Archive path updated to:\n{new_path}")
        except Exception as e:
            messagebox.showerror("Error", f"Cannot update config file: {e}")

    root.destroy()
