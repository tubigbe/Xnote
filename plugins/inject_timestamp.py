from datetime import datetime
import keyboard

HOTKEY = "alt+t"


def run():
    t = datetime.now()
    timestamp_str = f"{t.hour:02d}:{t.minute:02d} {t.year}/{t.month}/{t.day}\n\n"
    keyboard.write(timestamp_str)
