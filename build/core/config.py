import os
import sys
import platform
import configparser

from .constants import (
    GENERATORS, CONFIGURATIONS, PREMAKE_VERSIONS,
    KNOWN_PLUGINS, KNOWN_PLUGIN_URLS, KNOWN_PLUGIN_TYPES, KNOWN_PLUGIN_DEFAULT_INI,
    GITIGNORE_ENTRIES, SCRIPT_DIR, CONFIG_PATH, PROJECT_DIR,
)
from .utils import prompt, prompt_bool, prompt_list, update_gitignore

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
    "build":   ["generator", "configuration", "verbose", "bin_dir", "obj_dir"],
    "premake": ["dir", "exec", "file", "version"],
    "project": ["exec_name"],
}


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

    @staticmethod
    def _detect_exec_name():
        import re
        lua = os.path.join(PROJECT_DIR, "premake5.lua")
        if not os.path.exists(lua):
            return None
        with open(lua) as f:
            content = f.read()
        matches = re.findall(r'^\s*project\s+"([^"]+)"', content, re.MULTILINE)
        return matches[-1] if matches else None

    @classmethod
    def create_defaults(cls):
        exec_name = cls._detect_exec_name() or "MyProject"
        cls._write_ini(
            exec_name=exec_name,
            name=exec_name,
            run_dir="",
            run_args="",
            generator=GENERATORS[0],
            configuration=CONFIGURATIONS[0],
            verbose=False,
            dev=True,
            premake_version=PREMAKE_VERSIONS[0],
            plugins=KNOWN_PLUGINS,
        )
        update_gitignore(
            os.path.join(PROJECT_DIR, ".gitignore"),
            GITIGNORE_ENTRIES,
            "# --- build system ---",
        )
        from ..setup.premake import main as setup_premake
        setup_premake()

    @classmethod
    def _write_ini(cls, *, exec_name, name, run_dir, run_args, generator, configuration,
                   verbose, dev, premake_version, plugins):
        ini = configparser.ConfigParser()
        ini["build"] = {
            "generator":     generator,
            "configuration": configuration,
            "verbose":       str(verbose).lower(),
            "dev":           str(dev).lower(),
            "bin_dir":       "bin",
            "obj_dir":       "bin-int",
        }
        ini["premake"] = {
            "dir":        "premake",
            "exec":       "premake/premake5",
            "file":       "premake5.lua",
            "version":    premake_version,
            "plugin_dir": "premake/plugins",
        }
        ini["project"] = {
            "exec_name": exec_name,
            "name":      name,
            "run_dir":   run_dir,
            "run_args":  run_args,
        }
        for name in plugins:
            entry: dict = {"type": KNOWN_PLUGIN_TYPES.get(name, "premake")}
            url = KNOWN_PLUGIN_URLS.get(name, "")
            if url:
                entry["url"] = url
            entry.update(KNOWN_PLUGIN_DEFAULT_INI.get(name, {}))
            ini[f"plugin.{name}"] = entry
        with open(CONFIG_PATH, "w") as f:
            f.write("# Config file for the build system\n")
            f.write("# Project root is automatically inferred\n\n")
            ini.write(f)
        print(f"Config written to {CONFIG_PATH}")

    @classmethod
    def create(cls, force=False, yes=False):
        if os.path.exists(CONFIG_PATH) and not force:
            print("config.ini already exists. Use --force to overwrite.")
            sys.exit(1)

        print("Initialising build system config...\n")

        detected = cls._detect_exec_name()

        def _p(label, default=None, choices=None):
            return default if yes else prompt(label, default=default, choices=choices)

        def _pb(label, default=True):
            return default if yes else prompt_bool(label, default=default)

        def _pl(label, choices, defaults=None):
            return defaults if yes else prompt_list(label, choices, defaults=defaults)

        exec_name       = _p("Executable / project name", default=detected or "MyProject")
        name            = _p("Display name", default=exec_name)
        run_dir         = _p("Working directory when running (relative, blank = project root)", default="")
        run_args        = _p("Default run arguments (space-separated)", default="")
        generator       = _p("Premake generator", default="gmake", choices=GENERATORS)
        configuration   = _p("Default build configuration", default="Debug", choices=CONFIGURATIONS)
        verbose         = _pb("Verbose build output by default", default=False)
        dev             = _pb("Enable dev mode (compile commands) by default", default=True)
        premake_version = _p("Premake version", default=PREMAKE_VERSIONS[0], choices=PREMAKE_VERSIONS)
        plugins         = _pl("Plugins", KNOWN_PLUGINS, defaults=KNOWN_PLUGINS)

        cls._write_ini(
            exec_name=exec_name,
            name=name,
            run_dir=run_dir,
            run_args=run_args,
            generator=generator,
            configuration=configuration,
            verbose=verbose,
            dev=dev,
            premake_version=premake_version,
            plugins=plugins,
        )

        print(f"\nConfig written to {CONFIG_PATH}")

        if _pb("Update .gitignore", default=True):
            update_gitignore(
                os.path.join(PROJECT_DIR, ".gitignore"),
                GITIGNORE_ENTRIES,
                "# --- build system ---",
            )

        if _pb("Run setup now", default=True):
            from ..setup.premake import main as setup_premake
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
            print("Run `python build/ init` to regenerate.")
            sys.exit(1)

    def _load(self):
        if not os.path.exists(CONFIG_PATH):
            print("config.ini not found. Run `python build/ init` to create it.")
            sys.exit(1)

        ini = configparser.ConfigParser()
        ini.read(CONFIG_PATH)

        self._validate(ini)

        ini_build, ini_premake = ini["build"], ini["premake"]

        self.project_dir = PROJECT_DIR
        self.os_name     = self._get_os_name()
        self.arch        = self._get_arch()

        self.bin_dir      = os.path.join(self.project_dir, ini_build["bin_dir"])
        self.obj_dir      = os.path.join(self.project_dir, ini_build["obj_dir"])
        self.generator     = ini_build["generator"]
        self.configuration = ini_build["configuration"]
        self.verbose       = ini_build.getboolean("verbose")
        self.dev           = ini_build.getboolean("dev", fallback=False)

        self.premake_dir     = os.path.join(self.project_dir, ini_premake["dir"])
        self.premake_exec    = os.path.join(self.project_dir, ini_premake["exec"])
        self.premake_file    = os.path.join(self.project_dir, ini_premake["file"])
        self.premake_version = ini_premake["version"]
        self.plugin_dir      = os.path.join(self.project_dir, ini_premake.get("plugin_dir", "premake/plugins"))

        ini_project = ini["project"]
        self.exec_name = ini_project["exec_name"]
        self.name      = ini_project.get("name", self.exec_name)
        _run_dir       = ini_project.get("run_dir", "").strip().strip('"\'')
        self.run_dir   = os.path.join(self.project_dir, _run_dir) if _run_dir else self.project_dir
        _run_args      = ini_project.get("run_args", "").strip().strip('"\'')
        self.run_args  = _run_args.split() if _run_args else []

def get_cfg() -> 'Config':
    return Config()
