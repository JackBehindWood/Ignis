import os
import sys
import subprocess
import shutil
import argparse

from config import get_cfg
from plugin import plugins

def run_command(cmd, cwd=None, verbose=None):
    v = get_cfg().verbose if verbose is None else verbose
    if v:
        print(f"> {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        sys.exit(result.returncode)

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
    plugin = plugins.get("export-compile-commands")
    if not plugin or not plugin.is_installed():
        print("export-compile-commands plugin not available, skipping.")
        return
    
    compile_commands_output = os.path.join(get_cfg().premake_dir, "output", "compile_commands")
    compile_commands_root   = os.path.join(get_cfg().project_dir, "compile_commands")
 
    #TODO: For somereason shutils doesn't remove compile_commands_root, it only copies the data to compile_commands_output

    if os.path.exists(compile_commands_root):
        shutil.rmtree(compile_commands_root)
    if os.path.exists(compile_commands_output):
        shutil.rmtree(compile_commands_output)
 
    run_command(premake_cmd("export-compile-commands"), cwd=get_cfg().project_dir, verbose=verbose)
 
    # Move fresh compile_commands/ into premake/output/
    os.makedirs(os.path.dirname(compile_commands_output), exist_ok=True)
    shutil.move(compile_commands_root, compile_commands_output)

def build_project(configuration, verbose):
    cmd = ["make", f"CONFIG={configuration}"]
    if verbose:
        cmd.append("verbose=1")
    run_command(cmd, cwd=get_cfg().project_dir, verbose=verbose)

def clean_project():
    for d in [get_cfg().bin_dir, get_cfg().obj_dir]:
        if os.path.exists(d):
            shutil.rmtree(d)
            print(f"Removed {d}")

def main(clean=False, dev=None, configuration=None, verbose=None):
    configuration = configuration or get_cfg().configuration
    verbose = get_cfg().verbose if verbose is None else verbose
    dev = get_cfg().dev if dev is None else dev

    if clean:
        clean_project()
    generate_build_files(verbose)
    if dev:
        generate_compile_commands(verbose)
    build_project(configuration, verbose)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--dev", action="store_true", default=None)
    parser.add_argument("--no-dev", dest="dev", action="store_false")
    parser.add_argument("--config", choices=["Debug", "Release", "Distribution"], default=None)
    parser.add_argument("--verbose", action="store_true", default=None)
    args = parser.parse_args()
    main(clean=args.clean, dev=args.dev, configuration=args.config, verbose=args.verbose)