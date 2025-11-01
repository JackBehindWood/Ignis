import os
import sys
import subprocess
import configparser

# -----------------------------
# Load config
# -----------------------------
CONFIG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.ini")
config = configparser.ConfigParser()
config.read(CONFIG_PATH)

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN_DIR = os.path.join(PROJECT_DIR, config["paths"]["bin_dir"])
OBJ_DIR = os.path.join(PROJECT_DIR, config["paths"]["obj_dir"])
PREMAKE_EXEC = os.path.join(PROJECT_DIR, config["paths"]["premake_exec"])
PREMAKE_FILE = os.path.join(PROJECT_DIR, config["paths"]["premake_file"])
GENERATOR = config["build"]["generator"]
CONFIGURATION = config["build"]["configuration"]
VERBOSE = config["build"].getboolean("verbose")

# -----------------------------
# Utility function
# -----------------------------
def run_command(cmd, cwd=None):
    if VERBOSE:
        print(f"> {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        sys.exit(result.returncode)

# -----------------------------
# Generate build files
# -----------------------------
def generate_build_files():
    os.makedirs(BIN_DIR, exist_ok=True)
    os.makedirs(OBJ_DIR, exist_ok=True)
    run_command([PREMAKE_EXEC, GENERATOR, f"--file={PREMAKE_FILE}"], cwd=PROJECT_DIR)

# -----------------------------
# Build project
# -----------------------------
def build_project():
    # Run make inside the build directory (where makefiles are)
    if VERBOSE:
        print(f"> make CONFIG={CONFIGURATION}")
        run_command(["make", "verbose=1",f"CONFIG={CONFIGURATION}"], cwd=PROJECT_DIR)
    else:
        run_command(["make", f"CONFIG={CONFIGURATION}"], cwd=PROJECT_DIR)

# -----------------------------
# Main
# -----------------------------
def main():
    generate_build_files()
    build_project()

if __name__ == "__main__":
    main()
