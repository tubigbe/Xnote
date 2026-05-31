import os

# 遵循契约：声明热键与核心执行体
HOTKEY = "alt+shift+o"

def run():
    # 因为插件在 plugins/ 子目录下，所以需要向上找一级得到主程序根目录
    plugin_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(plugin_dir)
    
    if os.path.exists(project_dir):
        os.startfile(project_dir)