import os
import sys
import platform
import configparser

from constants import (
    GENERATORS, CONFIGURATIONS, PREMAKE_VERSIONS,
    KNOWN_PLUGINS, KNOWN_PLUGIN_URLS, GITIGNORE_ENTRIES,
    SCRIPT_DIR, CONFIG_PATH, PROJECT_DIR,
)

PLATFORM_MAP = {
    "darwin": "macosx",
    "linux":  "linux",
    "win32":  "windows",
}

ARCH_MAP = {
    "arm64":  "AARCH64",
    "x86_64": "x86_64",
    "AMD64":  "x86_64",
    "x86":    "x86",
}

REQUIRED = {
    "paths": ["bin_dir", "obj_dir", "premake_exec", "premake_file"],
    "build": ["generator", "configuration", "verbose"],
    "project": ["exec_name"],
    "setup": ["premake_version"],
}


# ---------------------------------------------------------------------------
# Prompt helpers
# ---------------------------------------------------------------------------

def _prompt(label, default=None, choices=None):
    hint = f" [{default}]" if default else ""
    if choices:
        hint = f" ({'/'.join(choices)}){hint}"
    while True:
        val = input(f"  {label}{hint}: ").strip()
        if not val and default is not None:
            return default
        if choices and val not in choices:
            print(f"    Must be one of: {', '.join(choices)}")
            continue
        if val:
            return val

def _prompt_bool(label, default=True):
    hint = "Y/n" if default else "y/N"
    val = input(f"  {label} [{hint}]: ").strip().lower()
    if not val:
        return default
    return val in ("y", "yes")

def _prompt_list(label, choices, defaults=None):
    defaults = defaults or []
    print(f"  {label} (space-separated, available: {', '.join(choices)})")
    print(f"    default: {' '.join(defaults) if defaults else 'none'}")
    val = input("  > ").strip()
    if not val:
        return defaults
    selected = [v for v in val.split() if v in choices]
    unknown  = [v for v in val.split() if v not in choices]
    if unknown:
        print(f"    Ignoring unknown: {', '.join(unknown)}")
    return selected


# ---------------------------------------------------------------------------
# .gitignore
# ---------------------------------------------------------------------------

def _update_gitignore():
    gitignore_path = os.path.join(PROJECT_DIR, ".gitignore")
    marker = "# --- build system ---"

    if os.path.exists(gitignore_path):
        existing = open(gitignore_path).read()
        if marker in existing:
            print(".gitignore already contains build system entries, skipping.")
            return
        mode, label = "a", "Appending"
    else:
        mode, label = "w", "Creating"

    print(f"{label} .gitignore entries...")
    with open(gitignore_path, mode) as f:
        f.write(f"\n{marker}\n")
        f.write("\n".join(GITIGNORE_ENTRIES) + "\n")


# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

class Config:
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._load()
        return cls._instance

    @classmethod
    def create(cls, force=False):
        if os.path.exists(CONFIG_PATH) and not force:
            print("config.ini already exists. Use --force to overwrite.")
            sys.exit(1)

        print("Initialising build system config...\n")

        exec_name       = _prompt("Executable / project name", default="MyProject")
        generator       = _prompt("Premake generator", default="gmake", choices=GENERATORS)
        configuration   = _prompt("Default build configuration", default="Debug", choices=CONFIGURATIONS)
        verbose         = _prompt_bool("Verbose build output by default", default=False)
        dev             = _prompt_bool("Enable dev mode (compile commands) by default", default=True)
        premake_version = _prompt("Premake version", default=PREMAKE_VERSIONS[0], choices=PREMAKE_VERSIONS)
        plugins         = _prompt_list("Plugins", KNOWN_PLUGINS, defaults=["export-compile-commands"])

        ini = configparser.ConfigParser()
        ini["paths"] = {
            "bin_dir":      "bin",
            "obj_dir":      "bin-int",
            "premake_exec": "premake/premake5",
            "premake_file": "premake5.lua",
            "premake_dir":  "premake",
        }
        ini["build"] = {
            "generator":     generator,
            "configuration": configuration,
            "verbose":       str(verbose).lower(),
            "dev":           str(dev).lower(),
        }
        ini["project"] = {
            "exec_name": exec_name,
        }
        ini["setup"] = {
            "premake_version": premake_version,
        }
        ini["plugins"] = {
            "plugin_dir": "premake/plugins",
        }
        for name in plugins:
            ini[f"plugin.{name}"] = {
                "url":  KNOWN_PLUGIN_URLS[name],
                "type": "premake",
            }

        with open(CONFIG_PATH, "w") as f:
            f.write("# Config file for the build system\n")
            f.write("# Project root is automatically inferred\n\n")
            ini.write(f)

        print(f"\nConfig written to {CONFIG_PATH}")

        if _prompt_bool("Update .gitignore", default=True):
            _update_gitignore()

        if _prompt_bool("Run setup now", default=True):
            from setup_premake import main as setup_premake
            setup_premake()

    def _get_os_name(self) -> str:
        os_name = PLATFORM_MAP.get(sys.platform)
        if not os_name:
            print(f"Unsupported platform: {sys.platform}")
            sys.exit(1)
        return os_name

    def _get_arch(self) -> str:
        return ARCH_MAP.get(platform.machine(), platform.machine().upper())

    def _validate(self, ini: configparser.ConfigParser):
        errors = []
        for section, keys in REQUIRED.items():
            if section not in ini:
                errors.append(f"  Missing section: [{section}]")
                continue
            for key in keys:
                if key not in ini[section]:
                    errors.append(f"  Missing key: [{section}] {key}")
        if errors:
            print("config.ini is invalid:")
            print("\n".join(errors))
            print("Run `python cli.py init` to regenerate.")
            sys.exit(1)

    def _load(self):
        if not os.path.exists(CONFIG_PATH):
            print("config.ini not found. Run `python cli.py init` to create it.")
            sys.exit(1)

        ini = configparser.ConfigParser()
        ini.read(CONFIG_PATH)

        self._validate(ini)

        ini_path, ini_build = ini["paths"], ini["build"]

        self.project_dir = PROJECT_DIR
        self.os_name     = self._get_os_name()
        self.arch        = self._get_arch()

        self.bin_dir      = os.path.join(self.project_dir, ini_path["bin_dir"])
        self.obj_dir      = os.path.join(self.project_dir, ini_path["obj_dir"])
        self.premake_exec = os.path.join(self.project_dir, ini_path["premake_exec"])
        self.premake_file = os.path.join(self.project_dir, ini_path["premake_file"])
        self.premake_dir  = os.path.join(self.project_dir, ini_path.get("premake_dir", "premake"))

        self.generator     = ini_build["generator"]
        self.configuration = ini_build["configuration"]
        self.verbose       = ini_build.getboolean("verbose")
        self.dev           = ini_build.getboolean("dev", fallback=False)

        self.premake_version = ini["setup"]["premake_version"]
        self.exec_name       = ini["project"]["exec_name"]

        self.plugin_dir = os.path.join(self.project_dir, ini["plugins"].get("plugin_dir", "premake/plugins")) if "plugins" in ini else ""

def get_cfg() -> 'Config':
    return Config()