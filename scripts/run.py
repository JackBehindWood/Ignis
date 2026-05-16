from config import get_cfg

import os
import stat
import argparse

def run_project(configuration=None):
    configuration = configuration or get_cfg().configuration
    exec_path = os.path.join(get_cfg().bin_dir, f"{configuration}-{get_cfg().os_name}-{get_cfg().arch}", get_cfg().exec_name, get_cfg().exec_name)
    if os.path.exists(exec_path):
        os.chmod(exec_path, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
        os.chdir(get_cfg().project_dir)
        os.execv(exec_path, [exec_path])
    else:
        print(f"Executable not found at {exec_path}. Make sure the build succeeded.")

def main(configuration=None):
    run_project(configuration)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", choices=["Debug", "Release", "Distribution"], default=None)
    args = parser.parse_args()
    main(configuration=args.config)