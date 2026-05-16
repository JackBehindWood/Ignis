import os

SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(SCRIPT_DIR, "config.ini")
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)

GENERATORS       = ["gmake", "gmake2", "xcode4", "vs2022", "vs2019"]
CONFIGURATIONS   = ["Debug", "Release", "Distribution"]
PREMAKE_VERSIONS = ["5.0.0-beta8", "5.0.0-beta7", "5.0.0-beta6"]
KNOWN_PLUGINS    = ["export-compile-commands"]
KNOWN_PLUGIN_URLS = {
    "export-compile-commands": "https://github.com/tarruda/premake-export-compile-commands",
}

GITIGNORE_ENTRIES = [
    "# Build output",
    "bin/",
    "bin-int/",
    "",
    "# Premake",
    "premake/",
    "",
    "# Python",
    "__pycache__/",
]