import os
import sys
import shutil
from abc import ABC, abstractmethod
from typing import Dict, Optional, Type

from ..core.config import get_cfg
from ..core.constants import CONFIG_PATH
from ..core.utils import run_command


class BasePlugin(ABC):
    def __init__(self, name: str, url: str):
        self.name = name
        self.url  = url
        self.path = os.path.join(get_cfg().plugin_dir, name)

    def is_installed(self) -> bool:
        return os.path.exists(self.path)

    @abstractmethod
    def install(self): ...

    @abstractmethod
    def update(self): ...

    def load_from_ini(self, section: dict):
        pass

    def add_cli(self, sub) -> Optional[str]:
        return None

    def run_cli(self, args) -> None:
        pass

    def uninstall(self):
        if not self.is_installed():
            print(f"Plugin '{self.name}' not installed.")
            return
        shutil.rmtree(self.path)
        print(f"Plugin '{self.name}' uninstalled.")


class PremakePlugin(BasePlugin):
    def install(self):
        if self.is_installed():
            print(f"Plugin '{self.name}' already installed.")
            return
        print(f"Installing plugin '{self.name}'...")
        os.makedirs(get_cfg().plugin_dir, exist_ok=True)
        run_command(["git", "clone", self.url, self.path])
        print(f"Plugin '{self.name}' installed.")

    def update(self):
        if not self.is_installed():
            self.install()
            return
        print(f"Updating plugin '{self.name}'...")
        run_command(["git", "-C", self.path, "pull"])
        print(f"Plugin '{self.name}' updated.")


class PythonPlugin(BasePlugin):
    def is_installed(self) -> bool:
        return True

    def install(self):
        print(f"Plugin '{self.name}' is built-in — no install needed.")

    def update(self):
        print(f"Plugin '{self.name}' is built-in — no update needed.")

    def load_from_ini(self, section: dict):
        pass


class VscodePlugin(PythonPlugin):
    def __init__(self, name: str, url: str):
        super().__init__(name, url)
        self.makefile_tools: bool = False
        self.build_log: str = ""

    def load_from_ini(self, section: dict):
        self.makefile_tools = section.get("makefile_tools", "false").lower() == "true"
        self.build_log = section.get("build_log", "").strip()

    def add_cli(self, sub) -> Optional[str]:
        from .vscode import add_args
        add_args(sub.add_parser("vscode", help="Generate (or clean) .vscode/ files"))
        return "vscode"

    def run_cli(self, args) -> None:
        from .vscode import main_from_args
        main_from_args(args)


PLUGIN_TYPES: Dict[str, Type[BasePlugin]] = {
    "premake": PremakePlugin,
    "python":  PythonPlugin,
    "vscode":  VscodePlugin,
}


class PluginManager:
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._plugins = {}
            cls._instance._load()
        return cls._instance

    def __init__(self):
        pass

    def _load(self):
        import configparser
        ini = configparser.ConfigParser()
        ini.read(CONFIG_PATH)

        for section in ini.sections():
            if not section.startswith("plugin."):
                continue
            name = section[len("plugin."):]
            url  = ini[section].get("url", "")
            kind = ini[section].get("type", "premake")
            cls_ = PLUGIN_TYPES.get(kind)
            if not cls_:
                print(f"Unknown plugin type '{kind}' for '{name}' — skipping.")
                continue
            if not url and kind not in ("python", "vscode"):
                print(f"No URL for plugin '{name}' — skipping.")
                continue
            plugin = cls_(name, url)
            plugin.load_from_ini(dict(ini[section]))
            self._plugins[name] = plugin

    def install_all(self):
        for p in self._plugins.values():
            p.install()

    def update_all(self):
        for p in self._plugins.values():
            p.update()

    def uninstall_all(self):
        for p in self._plugins.values():
            p.uninstall()

    def install(self, name: str):
        self._get_or_exit(name).install()

    def update(self, name: str):
        self._get_or_exit(name).update()

    def uninstall(self, name: str):
        self._get_or_exit(name).uninstall()

    def list(self) -> Dict[str, BasePlugin]:
        return dict(self._plugins)

    def get(self, name: str) -> Optional[BasePlugin]:
        return self._plugins.get(name)

    def _get_or_exit(self, name: str) -> BasePlugin:
        plugin = self._plugins.get(name)
        if not plugin:
            print(f"Unknown plugin '{name}'. Check your config.ini.")
            sys.exit(1)
        return plugin


plugins = PluginManager()
