import argparse
import os
import sys

from .core.constants import CONFIG_PATH, CONFIGURATIONS


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_init(args):
    from .core.config import Config
    Config.create(force=args.force, yes=args.yes)


def cmd_bootstrap(args):
    if not os.path.exists(CONFIG_PATH):
        print("No config.ini found — generating with defaults...")
        from .core.config import Config
        Config.create_defaults()

    _require_config()
    from .commands.setup import main as setup_main
    setup_main()

    from .commands.build import main as build_main
    build_main()

    print("\n✓ Bootstrap complete.")


def cmd_doctor(args):
    import subprocess
    checks = []

    def chk(label, ok):
        checks.append((label, ok))
        print(f"  {'✓' if ok else '✗'}  {label}")

    import sys as _sys
    v = _sys.version_info
    chk(f"Python {v.major}.{v.minor}.{v.micro}", True)

    has_config = os.path.exists(CONFIG_PATH)
    chk("config.ini", has_config)

    if not has_config:
        print("\nRun `make bootstrap` or `python build/ init` to set up.")
        return

    from .core.config import get_cfg
    cfg = get_cfg()

    premake_ok = os.path.exists(cfg.premake_exec) and os.access(cfg.premake_exec, os.X_OK)
    chk(f"premake5 ({cfg.premake_version})", premake_ok)

    dxc_ok = os.path.isdir(os.path.join(cfg.project_dir, "engine", "vendor", "dxc"))
    chk("DXC (engine/vendor/dxc/)", dxc_ok)

    from .plugins import plugins
    for name, p in plugins.list().items():
        chk(f"plugin: {name}", p.is_installed())

    cc_debug = os.path.join(cfg.premake_dir, "output", "compile_commands", "debug.json")
    chk("compile_commands/debug.json", os.path.exists(cc_debug))

    if plugins.get("vscode"):
        chk(".vscode/", os.path.isdir(os.path.join(cfg.project_dir, ".vscode")))

    if plugins.get("clangd"):
        chk("compile_commands.json (clangd symlink)",
            os.path.lexists(os.path.join(cfg.project_dir, "compile_commands.json")))

    r = subprocess.run(["git", "config", "core.hooksPath"],
                       capture_output=True, text=True, cwd=cfg.project_dir)
    chk("git hooks (.githooks)", r.returncode == 0 and ".githooks" in r.stdout)

    failures = [l for l, ok in checks if not ok]
    if failures:
        print(f"\n{len(failures)} check(s) failed. Run `make setup` to fix prerequisites.")
    else:
        print("\nAll checks passed.")

def cmd_setup(args):
    _require_config()
    from .commands.setup import main_from_args
    main_from_args(args)


def cmd_build(args):
    _require_config()
    from .commands.build import main_from_args
    main_from_args(args)


def cmd_run(args):
    _require_config()
    from .commands.run import main_from_args
    main_from_args(args)


def cmd_build_run(args):
    _require_config()
    from .commands.build import main_from_args as build_main
    from .commands.run   import main_from_args as run_main
    build_main(args)
    run_main(args)


def cmd_clean(args):
    _require_config()
    from .commands.build import clean_project
    from .core.config import get_cfg
    from .plugins import plugins
    import shutil

    clean_project()

    compile_commands_out = os.path.join(get_cfg().premake_dir, "output", "compile_commands")
    if os.path.exists(compile_commands_out):
        shutil.rmtree(compile_commands_out)
        print(f"Removed {compile_commands_out}")

    if plugins.get("clangd"):
        from .plugins.clangd import clean as clangd_clean
        clangd_clean()


def cmd_test(args):
    _require_config()
    from .commands.build import run_tests
    run_tests(configuration=args.config)


def cmd_setup_hooks(args):
    import subprocess
    from .core.config import get_cfg
    print("Setting up git hooks...")
    try:
        subprocess.check_call(["git", "config", "core.hooksPath", ".githooks"], cwd=get_cfg().project_dir)
        print("✓ Git hooks installed at .githooks/")
        print("  Pre-commit hook: auto-formats staged files with clang-format and re-stages them")
        print("  Tip: install clang-format if not present: brew install clang-format")
    except subprocess.CalledProcessError:
        print("✗ Failed to set up git hooks")
        sys.exit(1)


def cmd_plugin(args):
    _require_config()
    from .plugins import plugins

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
        print("No config.ini found. Run `python build/ init` first.")
        sys.exit(1)


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def _add_plugin_target(p):
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument("name", nargs="?", default=None)
    g.add_argument("--all", action="store_true")


def main():
    from .commands.build import add_args as _build_args
    from .commands.run   import add_args as _run_args
    from .commands.setup import add_args as _setup_args

    parser = argparse.ArgumentParser(prog="build", description="Ignis build system")
    sub = parser.add_subparsers(dest="command", required=True)

    p_init = sub.add_parser("init", help="Scaffold config.ini interactively")
    p_init.add_argument("--force", action="store_true", help="Overwrite existing config.ini")
    p_init.add_argument("--yes",   action="store_true", help="Accept all defaults without prompting")

    _setup_args(sub.add_parser("setup",     help="Install premake and plugins"))
    _build_args(sub.add_parser("build",     help="Generate build files and compile"))
    _run_args  (sub.add_parser("run",       help="Run the compiled executable"))
    _build_args(sub.add_parser("build-run", help="Build then run"))
    sub.add_parser("clean",     help="Remove build artifacts and compile commands")
    sub.add_parser("bootstrap", help="Init + setup + build in one shot (fresh checkout)")
    sub.add_parser("doctor",    help="Check all prerequisites and print status")

    p_test = sub.add_parser("test", help="Run tests")
    p_test.add_argument("--config", choices=CONFIGURATIONS, default=None)

    sub.add_parser("setup-hooks", help="Install git pre-commit hooks")

    p_plugin = sub.add_parser("plugin", help="Manage premake plugins")
    plugin_sub = p_plugin.add_subparsers(dest="plugin_command", required=True)
    plugin_sub.add_parser("list", help="List configured plugins and their status")
    _add_plugin_target(plugin_sub.add_parser("install",   help="Install a plugin or all plugins"))
    _add_plugin_target(plugin_sub.add_parser("update",    help="Update a plugin or all plugins"))
    _add_plugin_target(plugin_sub.add_parser("uninstall", help="Uninstall a plugin or all plugins"))

    dispatch = {
        "init":         cmd_init,
        "setup":        cmd_setup,
        "build":        cmd_build,
        "run":          cmd_run,
        "build-run":    cmd_build_run,
        "clean":        cmd_clean,
        "bootstrap":    cmd_bootstrap,
        "doctor":       cmd_doctor,
        "test":         cmd_test,
        "setup-hooks":  cmd_setup_hooks,
        "plugin":       cmd_plugin,
    }

    from .plugins import plugins as _plugins
    for _p in _plugins.list().values():
        _cmd_name = _p.add_cli(sub)
        if _cmd_name:
            def _make_cmd(p):
                def _cmd(args):
                    _require_config()
                    p.run_cli(args)
                return _cmd
            dispatch[_cmd_name] = _make_cmd(_p)

    args = parser.parse_args()
    dispatch[args.command](args)


main()
