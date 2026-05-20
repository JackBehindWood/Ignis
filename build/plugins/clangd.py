import os

from ..core.config import get_cfg

_SYMLINK_NAME = "compile_commands.json"
_CC_DEBUG_REL = os.path.join("premake", "output", "compile_commands", "debug.json")


def generate():
    cfg = get_cfg()
    dst = os.path.join(cfg.project_dir, _SYMLINK_NAME)
    src_abs = os.path.join(cfg.project_dir, _CC_DEBUG_REL)

    if not os.path.exists(src_abs):
        print("compile_commands not generated yet — skipping clangd symlink.")
        return

    if os.path.lexists(dst):
        os.remove(dst)

    os.symlink(_CC_DEBUG_REL, dst)
    print(f"Linked {_SYMLINK_NAME} → {_CC_DEBUG_REL}")


def clean():
    dst = os.path.join(get_cfg().project_dir, _SYMLINK_NAME)
    if os.path.lexists(dst):
        os.remove(dst)
        print(f"Removed {_SYMLINK_NAME}")
