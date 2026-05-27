import os
import shutil
import importlib.util
import sys

from ..core.config import get_cfg
from ..core.constants import CONFIGURATIONS
from ..core.utils import run_command


def premake_cmd(*args):
    cmd = [get_cfg().premake_exec]
    if get_cfg().plugin_dir:
        cmd.append(f"--scripts={get_cfg().plugin_dir}")
    cmd.extend(args)
    cmd.append(f"--file={get_cfg().premake_file}")
    return cmd

def generate_build_files(verbose):
    os.makedirs(get_cfg().bin_dir, exist_ok=True)
    os.makedirs(get_cfg().obj_dir, exist_ok=True)
    run_command(premake_cmd(get_cfg().generator), cwd=get_cfg().project_dir, verbose=verbose)

def generate_compile_commands(verbose):
    from ..plugins import plugins
    plugin = plugins.get("export-compile-commands")
    if not plugin or not plugin.is_installed():
        print("export-compile-commands plugin not available, skipping.")
        return

    compile_commands_output = os.path.join(get_cfg().premake_dir, "output", "compile_commands")
    compile_commands_root   = os.path.join(get_cfg().project_dir, "compile_commands")

    if os.path.exists(compile_commands_root):
        shutil.rmtree(compile_commands_root)
    if os.path.exists(compile_commands_output):
        shutil.rmtree(compile_commands_output)

    run_command(premake_cmd("export-compile-commands"), cwd=get_cfg().project_dir, verbose=verbose)

    os.makedirs(os.path.dirname(compile_commands_output), exist_ok=True)
    os.rename(compile_commands_root, compile_commands_output)

    if plugins.get("clangd"):
        from ..plugins.clangd import generate as clangd_generate
        clangd_generate()

def build_project(configuration, verbose):
    import os
    cmd = ["make", "-f", "Makefile", f"-j{os.cpu_count() or 4}", f"CONFIG={configuration}"]
    if verbose:
        cmd.append("verbose=1")
    run_command(cmd, cwd=get_cfg().project_dir, verbose=verbose)

def _run_iht(ctx):
    from pathlib import Path
    from ..iht import run as iht_run

    project_root = Path(ctx.project_dir)
    iht_run(
        project_root=project_root,
        scan_dirs=[project_root / "engine" / "src"],
        output_dir=project_root / "engine" / "generated",
        manifest_path=Path(ctx.obj_dir) / "iht_manifest.json",
    )

def clean_project():
    cfg = get_cfg()
    dirs = [
        cfg.bin_dir,
        cfg.obj_dir,
        os.path.join(cfg.project_dir, "engine", "generated"),
    ]
    for d in dirs:
        if os.path.exists(d):
            shutil.rmtree(d)
            print(f"Removed {d}")


# ---------------------------------------------------------------------------
# Hook system
# ---------------------------------------------------------------------------

class HookContext:
    def __init__(self, cfg, configuration):
        self.config = cfg
        self.configuration = configuration
        self.project_dir = cfg.project_dir
        self.bin_dir = cfg.bin_dir
        self.obj_dir = cfg.obj_dir
        self.verbose = cfg.verbose


def _run_hook(hook_name, ctx):
    hook_file = os.path.join(ctx.project_dir, "build", "hooks", f"{hook_name}.py")
    if not os.path.exists(hook_file):
        return

    try:
        spec = importlib.util.spec_from_file_location(hook_name, hook_file)
        if spec is None or spec.loader is None:
            return
        module = importlib.util.module_from_spec(spec)
        sys.modules[hook_name] = module
        spec.loader.exec_module(module)

        if hasattr(module, "run"):
            print(f"▶ Running {hook_name} hook...")
            module.run(ctx)
            print(f"✓ {hook_name} hook completed")
    except Exception as e:
        print(f"✗ Hook '{hook_name}' failed: {e}", file=sys.stderr)
        sys.exit(1)

def main(clean=False, dev=None, configuration=None, verbose=None):
    from ..plugins import plugins
    configuration = configuration or get_cfg().configuration
    verbose = get_cfg().verbose if verbose is None else verbose
    dev = get_cfg().dev if dev is None else dev

    ctx = HookContext(get_cfg(), configuration)

    if clean:
        clean_project()
    _run_iht(ctx)
    generate_build_files(verbose)
    if dev:
        generate_compile_commands(verbose)

    _run_hook("pre_build", ctx)
    build_project(configuration, verbose)
    _run_hook("post_build", ctx)

    if plugins.get("vscode") and not dev:
        from ..plugins.vscode import generate as vscode_generate
        vscode_generate()

def run_tests(configuration=None):
    configuration = configuration or get_cfg().configuration
    cmd = ["make", f"CONFIG={configuration}", "test"]
    print(f"Running tests (Config: {configuration})...")
    run_command(cmd, cwd=get_cfg().project_dir)
    print("Tests passed!")


def add_args(parser):
    parser.add_argument("--clean",   action="store_true")
    parser.add_argument("--dev",     action="store_true", default=None)
    parser.add_argument("--no-dev",  dest="dev", action="store_false")
    parser.add_argument("--config",  choices=CONFIGURATIONS, default=None)
    parser.add_argument("--verbose", action="store_true", default=None)


def main_from_args(args):
    main(clean=args.clean, dev=args.dev, configuration=args.config, verbose=args.verbose)
