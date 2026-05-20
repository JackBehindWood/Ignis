import os

_BUILD_DIR  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPT_DIR  = _BUILD_DIR
CONFIG_PATH = os.path.join(_BUILD_DIR, "config.ini")
PROJECT_DIR = os.path.dirname(_BUILD_DIR)

GENERATORS       = ["gmake", "gmake2", "xcode4", "vs2022", "vs2019"]
CONFIGURATIONS   = ["Debug", "Release", "Distribution"]
PREMAKE_VERSIONS = ["5.0.0-beta8", "5.0.0-beta7", "5.0.0-beta6"]
KNOWN_PLUGINS    = ["export-compile-commands", "vscode", "clangd"]
KNOWN_PLUGIN_URLS = {
    "export-compile-commands": "https://github.com/tarruda/premake-export-compile-commands",
    "vscode":  "",
    "clangd":  "",
}
KNOWN_PLUGIN_TYPES = {
    "export-compile-commands": "premake",
    "vscode":  "vscode",
    "clangd":  "python",
}

KNOWN_PLUGIN_DEFAULT_INI: dict[str, dict] = {
    "vscode": {"makefile_tools": "false", "build_log": ""},
}

GITIGNORE_ENTRIES = [
    "# Build output",
    "bin/",
    "bin-int/",
    "",
    "# Compile commands",
    "compile_commands.json",
    "compile_commands/",
    "",
    "# Premake",
    "premake/",
    "",
    "# Python",
    "__pycache__/",
]
