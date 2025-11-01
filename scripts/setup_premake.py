import subprocess
import shutil
import sys
import os
import configparser

# -----------------------------
# Load config
# -----------------------------
CONFIG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.ini")
config = configparser.ConfigParser()
config.read(CONFIG_PATH)

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PREMAKE_DIR = os.path.join(PROJECT_DIR, config["paths"].get("premake_dir", "premake"))
PREMAKE_EXEC = os.path.join(PREMAKE_DIR, "premake5")

# -----------------------------
# Check if Premake is installed
# -----------------------------
def is_premake_installed():
    return os.path.exists(PREMAKE_EXEC) and os.access(PREMAKE_EXEC, os.X_OK)

# -----------------------------
# Install Premake locally
# -----------------------------
def install_premake():
    print(f"Premake not found locally. Installing into {PREMAKE_DIR}...")
    
    # URL for macOS binary (adjust if needed)
    url = "https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-macosx.tar.gz"
    tar_file = os.path.join(PREMAKE_DIR, "premake.tar.gz")

    os.makedirs(PREMAKE_DIR, exist_ok=True)

    try:
        # Download
        subprocess.check_call(["curl", "-L", url, "-o", tar_file])
        # Extract
        subprocess.check_call(["tar", "-xzf", tar_file, "-C", PREMAKE_DIR, "--strip-components=1"])
        # Make executable
        subprocess.check_call(["chmod", "+x", PREMAKE_EXEC])
        # Remove tar
        os.remove(tar_file)
        print(f"Premake installed locally at {PREMAKE_EXEC}")
    except subprocess.CalledProcessError:
        print("Failed to install Premake. Please install manually.")
        sys.exit(1)

# -----------------------------
# Main
# -----------------------------
def main():
    if is_premake_installed():
        print(f"Premake is already installed locally at {PREMAKE_EXEC}")
    else:
        install_premake()

if __name__ == "__main__":
    main()
