import os
import sys
import json
import tkinter as tk
from tkinter import messagebox
from datetime import datetime

if getattr(sys, 'frozen', False):
    BASE_DIR = os.path.dirname(sys.executable)
else:
    BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

HOTKEY = "alt+o"
CONFIG_FILE = os.path.join(BASE_DIR, "config.json")


def run():
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                base_path = json.load(f).get("archive_path")

                if base_path and os.path.exists(base_path):
                    now = datetime.now()
                    folder_str = now.strftime("%Y.%m")
                    file_str = now.strftime("%Y.%m.%d")

                    target_path = os.path.join(base_path, folder_str)
                    if not os.path.exists(target_path):
                        os.makedirs(target_path)

                    file_name = f"{file_str}.txt"
                    file_path = os.path.join(target_path, file_name)
                    if not os.path.exists(file_path):
                        with open(file_path, "w", encoding="utf-8") as txt_file:
                            pass

                    os.startfile(target_path)
                    os.startfile(file_path)
                    return
        except Exception as e:
            print(f"Plugin error: {e}")

    root = tk.Tk()
    root.withdraw()
    messagebox.showerror("Error", "Cannot open folder or file — path is invalid or config is corrupted!")
    root.destroy()
