import os
import json
import glob
import importlib.util
import tkinter as tk
from tkinter import filedialog, messagebox

# 核心底层依赖库
import keyboard  
import pystray
from pystray import MenuItem as item
from PIL import Image, ImageDraw

CONFIG_FILE = "config.json"
PLUGIN_DIR = "plugins"         # 动态副函数扩展目录
HOTKEY_REGISTRY = {}           # 全局热键注册表

def get_or_create_config():
    """记忆中枢：负责读取或引导创建随笔总目录"""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                config = json.load(f)
                saved_path = config.get("archive_path")
                if saved_path and os.path.exists(saved_path):
                    return saved_path
        except Exception as e:
            print(f"读取配置失败: {e}")

    root = tk.Tk()
    root.withdraw()
    messagebox.showinfo("初始化提示", "老肥，请选择或创建你的随笔归档总目录")
    selected_path = filedialog.askdirectory(title="选择你的随笔归档总目录")
    
    if not selected_path:
        messagebox.showerror("错误", "必须指定一个有效的归档路径才能运行程序！")
        root.destroy()
        exit()
        
    try:
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            json.dump({"archive_path": selected_path}, f, ensure_ascii=False, indent=4)
    except Exception as e:
        messagebox.showerror("错误", f"无法写入配置文件: {e}")
        root.destroy()
        exit()
        
    root.destroy()
    return selected_path


def tray_open_folder(icon=None, item=None):
    """供托盘菜单原生调用的轻量级打开文件夹函数"""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                path = json.load(f).get("archive_path")
                if path and os.path.exists(path):
                    os.startfile(path)
                    return
        except:
            pass
    messagebox.showerror("错误", "无法打开文件夹，路径失效或配置文件丢失！")


def register_hotkey(hotkey_str, callback_func, owner_name):
    """统一热键注册中心（核心冲突判定拦截器）"""
    normalized_hotkey = hotkey_str.strip().lower().replace(" ", "")
    
    if normalized_hotkey in HOTKEY_REGISTRY:
        conflicting_owner = HOTKEY_REGISTRY[normalized_hotkey]
        root = tk.Tk()
        root.withdraw()
        messagebox.showerror(
            "热键冲突引发报错", 
            f"老肥，检测到系统级热键发生碰撞，程序终止加载！\n\n"
            f"冲突热键：{hotkey_str}\n"
            f"功能函数 1：{conflicting_owner}\n"
            f"功能函数 2：{owner_name}\n\n"
            f"请检查外部插件文件或修改内置热键冲突。"
        )
        root.destroy()
        return False
    
    HOTKEY_REGISTRY[normalized_hotkey] = owner_name
    keyboard.add_hotkey(normalized_hotkey, callback_func)
    return True


def load_dynamic_plugins():
    """动态插件加载引擎：自动扫描 plugins 文件夹并挂载功能"""
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
                register_hotkey(plugin_hotkey, module.run, f"外部功能文件 -> {file_name}")
        except Exception as e:
            print(f"解析外部功能文件 {file_name} 时发生内部错误: {e}")


def create_element_icon():
    """在内存中动态绘制一个 64x64 的深底白色'X'几何图标"""
    img = Image.new('RGB', (64, 64), '#222222')
    draw = ImageDraw.Draw(img)
    draw.line([(16, 16), (48, 48)], fill='#ffffff', width=6)
    draw.line([(48, 16), (16, 48)], fill='#ffffff', width=6)
    return img


def exit_program(icon, item):
    """托盘右键退出函数"""
    keyboard.remove_all_hotkeys()
    icon.stop()


if __name__ == "__main__":
    # 1. 路径初始化与检测
    initial_path = get_or_create_config()
    
    # 2. 启动时自动无感唤出文件夹
    try:
        os.startfile(initial_path)
    except:
        pass
    
    # 3. 扫描并动态注入 plugins 文件夹下的所有功能（内核不带任何预设热键）
    load_dynamic_plugins()
    
    # 4. 系统托盘常驻，进入主守护循环
    tray_menu = pystray.Menu(
        item('打开随笔文件夹', tray_open_folder),
        item('退出 Xnote', exit_program)
    )
    icon = pystray.Icon("Xnote", create_element_icon(), "Xnote Backend Active", menu=tray_menu)
    icon.run()