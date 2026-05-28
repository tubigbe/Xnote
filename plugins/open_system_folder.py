import os
import json
import tkinter as tk
from tkinter import messagebox

# 约定沟通格式一：热键定义
HOTKEY = "alt+o"
CONFIG_FILE = "config.json"

# 约定沟通格式二：核心执行体
def run():
    """快速唤出/打开脚本已经设置好的随笔存储路径"""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                path = json.load(f).get("archive_path")
                if path and os.path.exists(path):
                    os.startfile(path)
                    return
        except:
            pass
            
    root = tk.Tk()
    root.withdraw()
    messagebox.showerror("错误", "无法快捷打开文件夹，路径失效或配置文件损坏！")
    root.destroy()