from datetime import datetime
import keyboard

# 约定沟通格式一：热键定义
HOTKEY = "alt+t"

# 约定沟通格式二：核心执行体
def run():
    """直接在当前光标处模拟键入时间戳"""
    t = datetime.now()
    timestamp_str = f"{t.hour:02d}:{t.minute:02d} {t.year}/{t.month}/{t.day}\n\n"
    keyboard.write(timestamp_str)