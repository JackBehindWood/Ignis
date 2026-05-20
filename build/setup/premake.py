import os
import shutil
import argparse

from ..core.config import get_cfg
from ..core.utils import require_tool, run_command


# ---------------------------------------------------------------------------
# Preflight checks
# ---------------------------------------------------------------------------

def preflight():
    require_tool("git")
    if get_cfg().os_name != "windows":
        require_tool("curl")
        require_tool("tar")


# ---------------------------------------------------------------------------
# Premake install/update
# ---------------------------------------------------------------------------

def is_premake_installed():
    return os.path.exists(get_cfg().premake_exec) and os.access(get_cfg().premake_exec, os.X_OK)

def install_premake():
    archive  = f"premake-{get_cfg().premake_version}-{get_cfg().os_name}.tar.gz"
    url      = f"https://github.com/premake/premake-core/releases/download/v{get_cfg().premake_version}/{archive}"
    tar_file = os.path.join(get_cfg().premake_dir, "premake.tar.gz")

    print(f"Installing premake {get_cfg().premake_version} for {get_cfg().os_name}...")
    os.makedirs(get_cfg().premake_dir, exist_ok=True)

    run_command(["curl", "-L", url, "-o", tar_file])
    run_command(["tar", "-xzf", tar_file, "-C", get_cfg().premake_dir])
    os.remove(tar_file)
    if get_cfg().os_name != "windows":
        os.chmod(get_cfg().premake_exec, 0o755)
    print(f"Premake {get_cfg().premake_version} installed at {get_cfg().premake_exec}")

def update_premake():
    print(f"Updating premake to {get_cfg().premake_version}...")
    if os.path.exists(get_cfg().premake_dir):
        shutil.rmtree(get_cfg().premake_dir)
    install_premake()

def main(update=False):
    from ..plugins import plugins
    preflight()
    if update:
        update_premake()
        plugins.update_all()
    elif not is_premake_installed():
        install_premake()
        plugins.install_all()
    else:
        print(f"Premake already installed at {get_cfg().premake_exec}")
        plugins.install_all()
