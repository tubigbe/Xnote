import os
import json
import tkinter as tk
from tkinter import messagebox
from datetime import datetime

# 约定沟通格式一：热键定义
HOTKEY = "alt+o"
CONFIG_FILE = "config.json"

# 约定沟通格式二：核心执行体
def run():
    """快速唤出/打开当前年份+月份对应的二级文件夹及当天具体日期的随笔txt文件"""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                base_path = json.load(f).get("archive_path")
                
                if base_path and os.path.exists(base_path):
                    # 1. 动态捕获当前时间轴的不同维度
                    now = datetime.now()
                    folder_str = now.strftime("%Y.%m")       # 文件夹格式：2026.06
                    file_str = now.strftime("%Y.%m.%d")      # 文件名格式：2026.06.03
                    
                    # 2. 智能拼接并创建二级文件夹
                    target_path = os.path.join(base_path, folder_str)
                    if not os.path.exists(target_path):
                        os.makedirs(target_path)
                    
                    # 3. 智能拼接并创建对应的天级 .txt 文件（归档在月级文件夹内）
                    file_name = f"{file_str}.txt"
                    file_path = os.path.join(target_path, file_name)
                    if not os.path.exists(file_path):
                        with open(file_path, "w", encoding="utf-8") as txt_file:
                            pass
                    
                    # 4. 一步到位：打开文件夹并唤出当天日志
                    os.startfile(target_path)
                    os.startfile(file_path)
                    return
        except Exception as e:
            print(f"插件执行内部错误: {e}")
            pass
            
    # 异常兜底处理
    root = tk.Tk()
    root.withdraw()
    messagebox.showerror("错误", "无法快捷打开文件夹或文件，路径失效或配置文件损坏！")
    root.destroy()