import os
import json
import tkinter as tk
from tkinter import filedialog, messagebox

# 约定沟通格式一：热键定义
HOTKEY = "alt+p"
CONFIG_FILE = "config.json"

# 约定沟通格式二：核心执行体
def run():
    """动态修改随笔归档的文件路径并持久化写入JSON"""
    root = tk.Tk()
    root.withdraw()
    
    # 尝试读取当前路径以便在打开选择框时定位
    current_path = ""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                current_path = json.load(f).get("archive_path", "")
        except:
            pass
            
    new_path = filedialog.askdirectory(title="重新选择你的随笔归档总目录", initialdir=current_path)
    if new_path:
        try:
            with open(CONFIG_FILE, "w", encoding="utf-8") as f:
                json.dump({"archive_path": new_path}, f, ensure_ascii=False, indent=4)
            messagebox.showinfo("修改成功", f"老肥，归档根路径已切换至：\n{new_path}")
        except Exception as e:
            messagebox.showerror("错误", f"无法更新配置文件: {e}")
            
    root.destroy()