import json
import os
import shutil

from ..core.config import get_cfg
from . import VscodePlugin, plugins


_VSCODE_DIR = ".vscode"

def generate():
    cfg = get_cfg()
    vscode_dir = os.path.join(cfg.project_dir, _VSCODE_DIR)
    os.makedirs(vscode_dir, exist_ok=True)
    print("Generating .vscode/ ...")
    _write(vscode_dir, "extensions.json",       _extensions())
    _write(vscode_dir, "c_cpp_properties.json", _c_cpp_properties(cfg))
    _write(vscode_dir, "tasks.json",             _tasks(cfg))
    _write(vscode_dir, "launch.json",            _launch(cfg))
    _merge_settings(vscode_dir, cfg)


def clean():
    vscode_dir = os.path.join(get_cfg().project_dir, _VSCODE_DIR)
    if os.path.isdir(vscode_dir):
        shutil.rmtree(vscode_dir)
        print(f"Removed {_VSCODE_DIR}/")
    else:
        print(f"{_VSCODE_DIR}/ not found.")

def _extensions():
    recs = [
        "llvm-vs-code-extensions.vscode-clangd",
        "vadimcn.vscode-lldb",
    ]
    vscode = plugins.get("vscode")
    if isinstance(vscode, VscodePlugin) and vscode.makefile_tools:
        recs.append("ms-vscode.makefile-tools")
    return {"recommendations": recs}



def _c_cpp_properties(cfg):
    arch_map = {"AARCH64": "arm64", "x86_64": "x64", "x86": "x86"}
    os_map   = {"macosx": "macos", "linux": "linux", "windows": "windows"}
    arch         = arch_map.get(cfg.arch, "arm64")
    os_name      = os_map.get(cfg.os_name, "macos")
    intellisense = f"{os_name}-clang-{arch}"

    configurations = [
        {
            "name": c,
            "compileCommands": f"${{workspaceFolder}}/premake/output/compile_commands/{c.lower()}.json",
            "cppStandard": "c++20",
            "intelliSenseMode": intellisense,
        }
        for c in ["Debug", "Release", "Distribution"]
    ]

    return {"configurations": configurations, "version": 4}


def _tasks(cfg):
    configs = ["Debug", "Release", "Distribution"]
    tasks = []

    for c in configs:
        tasks.append({
            "label": f"Build ({c})",
            "type": "shell",
            "command": f"make build CONFIG={c}",
            "group": {"kind": "build", "isDefault": c == cfg.configuration},
            "problemMatcher": ["$gcc"],
        })

    tasks += [
        {
            "label": "Clean",
            "type": "shell",
            "command": "make clean",
            "group": "build",
            "problemMatcher": [],
        },
        {
            "label": f"Run ({cfg.configuration})",
            "type": "shell",
            "command": f"make run CONFIG={cfg.configuration}",
            "group": "test",
            "problemMatcher": [],
        },
        {
            "label": f"Build & Run ({cfg.configuration})",
            "type": "shell",
            "command": f"make br CONFIG={cfg.configuration}",
            "group": "test",
            "problemMatcher": ["$gcc"],
        },
    ]

    return {"version": "2.0.0", "tasks": tasks}


def _launch(cfg):
    configurations = []
    for c in ["Debug", "Release", "Distribution"]:
        exe = "/".join([
            "${workspaceFolder}",
            "bin",
            f"{c}-{cfg.os_name}-{cfg.arch}",
            cfg.exec_name,
            cfg.exec_name,
        ])
        configurations.append({
            "name": f"{cfg.exec_name} ({c})",
            "type": "lldb",
            "request": "launch",
            "program": exe,
            "args": [],
            "cwd": "${workspaceFolder}",
            "preLaunchTask": f"Build ({c})",
        })

    return {"version": "0.2.0", "configurations": configurations}


def _merge_settings(vscode_dir, cfg):
    settings_path = os.path.join(vscode_dir, "settings.json")
    existing = {}
    if os.path.exists(settings_path):
        with open(settings_path) as f:
            try:
                existing = json.load(f)
            except json.JSONDecodeError:
                pass

    managed_keys = [
        "makefile.configurations",
        "makefile.defaultConfiguration",
        "makefile.buildTarget",
        "makefile.makefilePath",
        "makefile.launchConfigurations",
        "makefile.buildLog",
    ]
    for k in managed_keys:
        existing.pop(k, None)

    vscode = plugins.get("vscode")
    if isinstance(vscode, VscodePlugin) and vscode.makefile_tools:
        existing["makefile.configurations"] = [
            {"name": "Debug",        "makeArgs": ["CONFIG=Debug"]},
            {"name": "Release",      "makeArgs": ["CONFIG=Release"]},
            {"name": "Distribution", "makeArgs": ["CONFIG=Distribution"]},
        ]
        existing["makefile.defaultConfiguration"] = cfg.configuration
        existing["makefile.buildTarget"] = "build"
        existing["makefile.makefilePath"] = "GNUmakefile"
        existing["makefile.launchConfigurations"] = [
            {
                "cwd": "${workspaceFolder}",
                "binaryPath": f"bin/{c}-{cfg.os_name}-{cfg.arch}/{cfg.exec_name}/{cfg.exec_name}",
                "binaryArgs": [],
            }
            for c in ["Debug", "Release", "Distribution"]
        ]
        if vscode.build_log:
            existing["makefile.buildLog"] = vscode.build_log

    with open(settings_path, "w") as f:
        json.dump(existing, f, indent=4)
    print(f"  {_VSCODE_DIR}/settings.json")

def _write(dir_, name, data):
    with open(os.path.join(dir_, name), "w") as f:
        json.dump(data, f, indent=4)
    print(f"  {_VSCODE_DIR}/{name}")


def add_args(parser):
    parser.add_argument("--clean", action="store_true", help="Remove .vscode/ directory")


def main_from_args(args):
    if args.clean:
        clean()
    else:
        generate()


