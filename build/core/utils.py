import os
import sys
import shutil
import subprocess


def run_command(cmd, cwd=None, verbose=False):
    if verbose:
        print(f"> {' '.join(str(c) for c in cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        sys.exit(result.returncode)


def require_tool(name):
    if shutil.which(name) is None:
        print(f"Required tool not found: '{name}'. Please install it and re-run.")
        sys.exit(1)


def download_file(url, dest):
    print(f"Downloading {os.path.basename(str(dest))} ...")
    run_command(["curl", "-fsSL", "-o", str(dest), url])


# ---------------------------------------------------------------------------
# Interactive prompts
# ---------------------------------------------------------------------------

def prompt(label, default=None, choices=None):
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


def prompt_bool(label, default=True):
    hint = "Y/n" if default else "y/N"
    val = input(f"  {label} [{hint}]: ").strip().lower()
    if not val:
        return default
    return val in ("y", "yes")


def prompt_list(label, choices, defaults=None):
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
# .gitignore helpers
# ---------------------------------------------------------------------------

def update_gitignore(path, entries, marker):
    existing = open(path).read() if os.path.exists(path) else ""
    if marker in existing:
        print(".gitignore already contains build system entries, skipping.")
        return
    mode, label = ("a", "Appending") if os.path.exists(path) else ("w", "Creating")
    print(f"{label} .gitignore entries...")
    with open(path, mode) as f:
        f.write(f"\n{marker}\n")
        f.write("\n".join(entries) + "\n")


def ensure_gitignored(path, entry, comment=None):
    existing = open(path).read() if os.path.exists(path) else ""
    if entry in existing:
        return
    with open(path, "a") as f:
        if comment:
            f.write(f"\n{comment}\n{entry}\n")
        else:
            f.write(f"\n{entry}\n")
    print(f"Added '{entry}' to .gitignore")
