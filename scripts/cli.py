import argparse
import os
import sys

from constants import CONFIGURATIONS, CONFIG_PATH

# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_init(args):
    from config import Config
    Config.create(force=args.force)

def cmd_setup(args):
    _require_config()
    from setup import main as setup_project
    setup_project(update=args.update)


def cmd_build(args):
    _require_config()
    from build import main as build_project
    build_project(
        clean=args.clean,
        dev=args.dev,
        configuration=args.config,
        verbose=args.verbose,
    )


def cmd_run(args):
    _require_config()
    from run import main as run_project
    run_project(configuration=args.config)


def cmd_build_run(args):
    _require_config()
    from build import main as build_project
    from run import main as run_project
    build_project(
        clean=args.clean,
        dev=args.dev,
        configuration=args.config,
        verbose=args.verbose,
    )
    run_project(configuration=args.config)


def cmd_clean(args):
    _require_config()
    from build import clean_project
    from config import get_cfg
    import shutil

    clean_project()

    compile_commands_link = os.path.join(get_cfg().project_dir, "compile_commands")
    compile_commands_out  = os.path.join(get_cfg().premake_dir, "output", "compile_commands")

    if os.path.islink(compile_commands_link):
        os.unlink(compile_commands_link)
        print(f"Removed symlink {compile_commands_link}")
    if os.path.exists(compile_commands_out):
        shutil.rmtree(compile_commands_out)
        print(f"Removed {compile_commands_out}")


def cmd_plugin(args):
    _require_config()
    from plugin import plugins

    if args.plugin_command == "list":
        all_plugins = plugins.list()
        if not all_plugins:
            print("No plugins configured.")
            return
        for name, p in all_plugins.items():
            status = "installed" if p.is_installed() else "not installed"
            print(f"  {name:30s} [{status}]  {p.url}")

    elif args.plugin_command == "install":
        if args.all:
            plugins.install_all()
        else:
            plugins.install(args.name)

    elif args.plugin_command == "update":
        if args.all:
            plugins.update_all()
        else:
            plugins.update(args.name)

    elif args.plugin_command == "uninstall":
        if args.all:
            plugins.uninstall_all()
        else:
            plugins.uninstall(args.name)


# ---------------------------------------------------------------------------
# Guards
# ---------------------------------------------------------------------------

def _require_config():
    if not os.path.exists(CONFIG_PATH):
        print("No config.ini found. Run `python cli.py init` first.")
        sys.exit(1)


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def _add_build_args(p):
    p.add_argument("--clean",   action="store_true")
    p.add_argument("--dev",     action="store_true", default=None)
    p.add_argument("--no-dev",  dest="dev", action="store_false")
    p.add_argument("--config",  choices=CONFIGURATIONS, default=None)
    p.add_argument("--verbose", action="store_true", default=None)

def _add_plugin_target(p):
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument("name", nargs="?", default=None)
    g.add_argument("--all", action="store_true")


def main():
    parser = argparse.ArgumentParser(prog="cli", description="Ignis build system")
    sub = parser.add_subparsers(dest="command", required=True)

    p_init = sub.add_parser("init", help="Scaffold config.ini interactively")
    p_init.add_argument("--force", action="store_true", help="Overwrite existing config.ini")

    p_setup = sub.add_parser("setup", help="Install premake and plugins")
    p_setup.add_argument("--update", action="store_true", help="Force reinstall at version in config.ini")

    _add_build_args(sub.add_parser("build", help="Generate build files and compile"))

    p_run = sub.add_parser("run", help="Run the compiled executable")
    p_run.add_argument("--config", choices=CONFIGURATIONS, default=None)

    _add_build_args(sub.add_parser("build-run", help="Build then run"))

    sub.add_parser("clean", help="Remove build artifacts and compile commands")

    p_plugin = sub.add_parser("plugin", help="Manage premake plugins")
    plugin_sub = p_plugin.add_subparsers(dest="plugin_command", required=True)
    plugin_sub.add_parser("list", help="List configured plugins and their status")
    _add_plugin_target(plugin_sub.add_parser("install",   help="Install a plugin or all plugins"))
    _add_plugin_target(plugin_sub.add_parser("update",    help="Update a plugin or all plugins"))
    _add_plugin_target(plugin_sub.add_parser("uninstall", help="Uninstall a plugin or all plugins"))

    args = parser.parse_args()

    dispatch = {
        "init":      cmd_init,
        "setup":     cmd_setup,
        "build":     cmd_build,
        "run":       cmd_run,
        "build-run": cmd_build_run,
        "clean":     cmd_clean,
        "plugin":    cmd_plugin,
    }
    dispatch[args.command](args)


if __name__ == "__main__":
    main()