#!/usr/bin/env python3
import os
import stat
import configparser

# -----------------------------
# Load config
# -----------------------------
CONFIG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.ini")
config = configparser.ConfigParser()
config.read(CONFIG_PATH)

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN_DIR = os.path.join(PROJECT_DIR, config["paths"]["bin_dir"])
CONFIGURATION = config["build"]["configuration"]
EXEC_NAME = config["project"]["exec_name"]

# -----------------------------
# Run executable
# -----------------------------
def run_project():
    exec_path = os.path.join(
        BIN_DIR,
        f"{CONFIGURATION}-macosx-ARM64",
        EXEC_NAME,
        EXEC_NAME
    )

    if os.path.exists(exec_path):
        os.chmod(exec_path, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
        os.chdir(PROJECT_DIR)
        os.execv(exec_path, [exec_path])
    else:
        print(f"Executable not found at {exec_path}. Make sure the build succeeded.")

# -----------------------------
# Main
# -----------------------------
def main():
    run_project()

if __name__ == "__main__":
    main()
