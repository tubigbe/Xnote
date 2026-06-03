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
    """快速唤出/打开当前年份+月份对应的二级随笔归档文件夹，具备自动创建功能"""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                base_path = json.load(f).get("archive_path")
                
                if base_path and os.path.exists(base_path):
                    # 1. 动态捕获当前时间轴，生成 “年份.月份” 格式（例如 2026.06）
                    current_date_str = datetime.now().strftime("%Y.%m")
                    
                    # 2. 智能拼接二级文件夹路径
                    target_path = os.path.join(base_path, current_date_str)
                    
                    # 3. 自愈容错设计：如果新月份的文件夹不存在，静默自动创建
                    if not os.path.exists(target_path):
                        os.makedirs(target_path)
                    
                    # 4. 精准唤出目标系统文件夹
                    os.startfile(target_path)
                    return
        except Exception as e:
            # 允许打印日志以便在控制台或调试时观察
            print(f"插件执行内部错误: {e}")
            pass
            
    # 异常兜底处理
    root = tk.Tk()
    root.withdraw()
    messagebox.showerror("错误", "无法快捷打开文件夹，路径失效或配置文件损坏！")
    root.destroy()