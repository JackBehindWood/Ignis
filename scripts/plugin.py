import os
import sys
import shutil
import subprocess
from abc import ABC, abstractmethod
from typing import Dict, Type

from config import get_cfg


# ---------------------------------------------------------------------------
# Base
# ---------------------------------------------------------------------------

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

    def uninstall(self):
        if not self.is_installed():
            print(f"Plugin '{self.name}' not installed.")
            return
        shutil.rmtree(self.path)
        print(f"Plugin '{self.name}' uninstalled.")


# ---------------------------------------------------------------------------
# Types
# ---------------------------------------------------------------------------

class PremakePlugin(BasePlugin):
    def install(self):
        if self.is_installed():
            print(f"Plugin '{self.name}' already installed.")
            return
        print(f"Installing plugin '{self.name}'...")
        os.makedirs(get_cfg().plugin_dir, exist_ok=True)
        try:
            subprocess.check_call(["git", "clone", self.url, self.path])
            print(f"Plugin '{self.name}' installed.")
        except subprocess.CalledProcessError:
            print(f"Failed to install plugin '{self.name}'.")
            sys.exit(1)

    def update(self):
        if not self.is_installed():
            self.install()
            return
        print(f"Updating plugin '{self.name}'...")
        try:
            subprocess.check_call(["git", "-C", self.path, "pull"])
            print(f"Plugin '{self.name}' updated.")
        except subprocess.CalledProcessError:
            print(f"Failed to update plugin '{self.name}'.")
            sys.exit(1)


PLUGIN_TYPES: Dict[str, Type[BasePlugin]] = {
    "premake": PremakePlugin,
}


# ---------------------------------------------------------------------------
# Manager
# ---------------------------------------------------------------------------

class PluginManager:
    _instance = None

    def __init__(self):
        if hasattr(self, "_plugins"):
            return
        
        self._plugins: Dict[str, BasePlugin] = {}
        
    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._plugins = {}
            cls._instance._load()
        return cls._instance

    def _load(self):
        import configparser
        ini = configparser.ConfigParser()
        ini.read(os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.ini"))

        for section in ini.sections():
            if not section.startswith("plugin."):
                continue
            name     = section[len("plugin."):]
            url      = ini[section].get("url", "")
            kind     = ini[section].get("type", "premake")
            cls_     = PLUGIN_TYPES.get(kind)
            if not cls_:
                print(f"Unknown plugin type '{kind}' for '{name}' — skipping.")
                continue
            if not url:
                print(f"No URL for plugin '{name}' — skipping.")
                continue
            self._plugins[name] = cls_(name, url)

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

    def get(self, name: str) -> BasePlugin | None:
        return self._plugins.get(name)

    def _get_or_exit(self, name: str) -> BasePlugin:
        plugin = self._plugins.get(name)
        if not plugin:
            print(f"Unknown plugin '{name}'. Check your config.ini.")
            sys.exit(1)
        return plugin


plugins = PluginManager()