import os

HOTKEY = "alt+shift+o"


def run():
    plugin_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(plugin_dir)

    if os.path.exists(project_dir):
        os.startfile(project_dir)
