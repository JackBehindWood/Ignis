import os
import stat

from ..core.constants import CONFIGURATIONS
from ..core.config import get_cfg

def run_project(configuration=None):
    cfg = get_cfg()
    configuration = configuration or cfg.configuration
    exec_path = os.path.join(cfg.bin_dir, f"{configuration}-{cfg.os_name}-{cfg.arch}", cfg.exec_name, cfg.exec_name)
    if os.path.exists(exec_path):
        os.chmod(exec_path, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
        os.chdir(cfg.run_dir)
        os.execv(exec_path, [exec_path] + cfg.run_args)
    else:
        print(f"Executable not found at {exec_path}. Make sure the build succeeded.")

def add_args(parser):
    parser.add_argument("--config", choices=CONFIGURATIONS, default=None)


def main(configuration=None):
    run_project(configuration)


def main_from_args(args):
    main(configuration=args.config)
