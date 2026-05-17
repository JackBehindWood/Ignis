"""
setup_dxc.py  –  Installs DXC (DirectX Shader Compiler) into engine/vendor/dxc/.

Platform strategy:
  macOS  : Headers + libdxcompiler.dylib copied from a local LunarG Vulkan SDK install.
           The SDK is the only reliable source of both the macOS dylib and the non-Windows
           WinAdapter.h header (required by dxcapi.h on non-Windows platforms).
           Falls back to extracting headers from the GitHub Windows zip if the SDK's
           include/dxc/ directory is absent (e.g. minimal SDK install).
  Linux  : Pre-built tar from GitHub releases  (linux_dxc_*.tar.gz).
  Windows: Pre-built zip from GitHub releases  (dxc_*.zip).

Usage:
    python scripts/setup_dxc.py           # install (skip if already present)
    python scripts/setup_dxc.py --update  # force re-install
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent

GITHUB_API_URL = "https://api.github.com/repos/microsoft/DirectXShaderCompiler/releases/latest"

# ---------------------------------------------------------------------------
# Project directory — prefer the Config singleton when config.ini exists so
# this script stays consistent with the rest of the build tooling.
# Falls back to inferring the root from the script location when config.ini
# has not been created yet (e.g. first-time standalone run).
# ---------------------------------------------------------------------------

def _get_project_dir():
    config_ini = SCRIPT_DIR / "config.ini"
    if config_ini.exists():
        try:
            from config import get_cfg
            return Path(get_cfg().project_dir)
        except Exception:
            pass
    return SCRIPT_DIR.parent

PROJECT_DIR = _get_project_dir()
VENDOR_DIR  = PROJECT_DIR / "engine" / "vendor" / "dxc"

# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

def _require_tool(name):
    if shutil.which(name) is None:
        print(f"Required tool not found: '{name}'. Please install it and re-run.")
        sys.exit(1)

def preflight():
    _require_tool("git")
    _require_tool("curl")

def _run(*args, **kwargs):
    subprocess.check_call(list(args), **kwargs)

def _fetch_latest_release():
    print("Fetching latest DXC release from GitHub ...")
    result = subprocess.run(
        ["curl", "-fsSL", "-H", "Accept: application/vnd.github+json", GITHUB_API_URL],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        sys.exit(f"curl failed: {result.stderr.strip()}")
    return json.loads(result.stdout)

def _download(url, dest):
    print(f"Downloading {Path(dest).name} ...")
    _run("curl", "-fsSL", "-o", str(dest), url)

# ---------------------------------------------------------------------------
# macOS: headers + dylib from the LunarG Vulkan SDK
# ---------------------------------------------------------------------------

def _find_vulkan_sdk():
    """Return the Vulkan SDK macOS root if libdxcompiler.dylib is present, else None."""
    # 1. Explicit env var — set by the Vulkan SDK installer's setup-env.sh
    sdk_env = os.environ.get("VULKAN_SDK")
    if sdk_env:
        p = Path(sdk_env)
        if (p / "lib" / "libdxcompiler.dylib").exists():
            return p

    # 2. Default install location: ~/VulkanSDK/<version>/macOS/
    sdk_root = Path.home() / "VulkanSDK"
    if sdk_root.is_dir():
        versions = sorted(
            (v for v in sdk_root.iterdir() if v.is_dir()),
            key=lambda p: p.name,
            reverse=True,
        )
        for version_dir in versions:
            candidate = version_dir / "macOS"
            if (candidate / "lib" / "libdxcompiler.dylib").exists():
                return candidate

    return None

def _install_headers_from_sdk(sdk):
    """
    Copy DXC headers from the Vulkan SDK into engine/vendor/dxc/include/dxc/.
    The SDK ships them at $SDK/include/dxc/ which includes WinAdapter.h — the
    compatibility shim required by dxcapi.h on non-Windows platforms.
    Returns True on success, False if the directory is absent in this SDK install.
    """
    sdk_include = sdk / "include" / "dxc"
    if not sdk_include.is_dir():
        return False

    dest = VENDOR_DIR / "include" / "dxc"
    if dest.exists():
        shutil.rmtree(dest)
    shutil.copytree(sdk_include, dest)

    count = sum(1 for _ in dest.rglob("*") if _.is_file())
    print(f"  Copied {count} header file(s) from Vulkan SDK (includes WinAdapter.h)")
    return True

def _install_headers_from_zip(assets):
    """
    Fallback: extract DXC headers from the Windows GitHub zip.
    Used when the Vulkan SDK is present (for the dylib) but its include/dxc/
    directory is missing (minimal SDK install).  Maps inc/ -> include/dxc/.
    Note: some SDK versions do not ship WinAdapter.h in this zip; prefer the
    SDK path whenever possible.
    """
    win_zip = next(
        (a for a in assets
         if a["name"].endswith(".zip") and not a["name"].startswith("pdb")),
        None,
    )
    if win_zip is None:
        print("  No Windows zip asset found to extract headers from.")
        return False

    tmp = PROJECT_DIR / win_zip["name"]
    _download(win_zip["browser_download_url"], tmp)

    include_dest = VENDOR_DIR / "include" / "dxc"
    include_dest.mkdir(parents=True, exist_ok=True)

    print("Extracting headers from Windows zip (inc/ -> include/dxc/) ...")
    with zipfile.ZipFile(tmp) as zf:
        entries = [e for e in zf.namelist() if e.startswith("inc/") and not e.endswith("/")]
        for entry in entries:
            rel  = entry[len("inc/"):]
            dest = include_dest / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            with zf.open(entry) as src, open(dest, "wb") as out:
                shutil.copyfileobj(src, out)
        print(f"  {len(entries)} header file(s) installed")

    tmp.unlink()
    return True

def _install_macos(assets):
    sdk = _find_vulkan_sdk()
    if sdk is None:
        print(
            "\nLunarG Vulkan SDK not found.\n"
            "The SDK is required on macOS to supply both libdxcompiler.dylib and\n"
            "WinAdapter.h (the non-Windows compatibility header for dxcapi.h).\n"
            "Install it from:  https://vulkan.lunarg.com/sdk/home#mac\n"
            "Then re-run:      python scripts/setup_dxc.py --update"
        )
        return False

    print(f"Vulkan SDK found: {sdk}")

    # Prefer SDK headers (correct structure + WinAdapter.h guaranteed).
    # Fall back to the Windows zip only if the SDK's include/dxc/ is absent.
    if not _install_headers_from_sdk(sdk):
        print("  SDK include/dxc/ not found — falling back to Windows zip for headers ...")
        if not _install_headers_from_zip(assets):
            print("  Could not obtain DXC headers.")
            return False

    lib_dst = VENDOR_DIR / "lib"
    lib_dst.mkdir(parents=True, exist_ok=True)
    shutil.copy2(sdk / "lib" / "libdxcompiler.dylib", lib_dst / "libdxcompiler.dylib")
    print("  Copied libdxcompiler.dylib")
    return True

# ---------------------------------------------------------------------------
# Linux / Windows: pre-built binary from GitHub releases
# ---------------------------------------------------------------------------

_PLATFORM_KEYWORDS = {
    "linux": ["linux"],
    "win32": ["windows", "win64", "win32"],
}

_ARCH_KEYWORDS = {
    "arm64":  ["arm64", "aarch64"],
    "x86_64": ["x86_64", "x64", "amd64"],
    "amd64":  ["x86_64", "x64", "amd64"],
}

def _pick_asset(assets, platform_key, arch_key):
    plat_kws = _PLATFORM_KEYWORDS.get(platform_key, [])
    arch_kws = _ARCH_KEYWORDS.get(arch_key, [])

    for asset in assets:
        name = asset["name"].lower()
        if any(p in name for p in plat_kws) and any(a in name for a in arch_kws):
            return asset
    for asset in assets:
        name = asset["name"].lower()
        if any(p in name for p in plat_kws):
            return asset
    if platform_key == "win32":
        for asset in assets:
            if asset["name"].endswith(".zip") and not asset["name"].startswith("pdb"):
                return asset
    return None

def _install_from_release(assets, platform_key, arch_key):
    asset = _pick_asset(assets, platform_key, arch_key)
    if asset is None:
        print("Available assets:")
        for a in assets:
            print(f"  {a['name']}")
        print(f"No matching asset for {platform_key}/{arch_key}.")
        return False

    print(f"Matched asset: {asset['name']}")
    tmp = PROJECT_DIR / asset["name"]
    _download(asset["browser_download_url"], tmp)

    VENDOR_DIR.mkdir(parents=True, exist_ok=True)
    if asset["name"].endswith(".zip"):
        _require_tool("unzip")
        _run("unzip", "-q", "-o", str(tmp), "-d", str(VENDOR_DIR))
    else:
        _run("tar", "-xzf", str(tmp), "-C", str(VENDOR_DIR))

    tmp.unlink()
    return True

# ---------------------------------------------------------------------------
# .gitignore
# ---------------------------------------------------------------------------

def _ensure_gitignored():
    gitignore = PROJECT_DIR / ".gitignore"
    entry     = "engine/vendor/dxc/"
    if gitignore.exists() and entry in gitignore.read_text():
        return
    with open(gitignore, "a") as f:
        f.write(f"\n# DXC pre-built binaries (downloaded by setup_dxc.py)\n{entry}\n")
    print(f"Added '{entry}' to .gitignore")

# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main(update=False):
    preflight()

    if VENDOR_DIR.exists() and not update:
        print(f"DXC already installed at {VENDOR_DIR}  (pass --update to re-install)")
        return

    if VENDOR_DIR.exists() and update:
        print(f"Removing existing installation at {VENDOR_DIR} ...")
        shutil.rmtree(VENDOR_DIR)

    import platform as _platform
    arch_key = _platform.machine().lower()

    release = _fetch_latest_release()
    tag     = release.get("tag_name", "unknown")
    assets  = release.get("assets", [])
    print(f"Latest release: {tag}  ({len(assets)} asset(s))")

    if sys.platform == "darwin":
        success = _install_macos(assets)
    else:
        success = _install_from_release(assets, sys.platform, arch_key)

    if not success:
        sys.exit(1)

    _ensure_gitignored()

    print(f"\nDXC installed at {VENDOR_DIR}")
    print("Layout:")
    print("  engine/vendor/dxc/include/dxc/dxcapi.h")
    print("  engine/vendor/dxc/include/dxc/WinAdapter.h")
    print("  engine/vendor/dxc/lib/libdxcompiler.dylib  (macOS)")
    print("  engine/vendor/dxc/lib/libdxcompiler.so     (Linux)")
    print("  engine/vendor/dxc/lib/dxcompiler.dll       (Windows)")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Install DXC into engine/vendor/dxc/")
    parser.add_argument("--update", action="store_true", help="Remove and re-install even if already present")
    args = parser.parse_args()
    main(update=args.update)
