import argparse
import json
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

from ..core.utils import require_tool, run_command, download_file, ensure_gitignored

_BUILD_DIR = Path(__file__).resolve().parent.parent   # build/setup/../ = build/

GITHUB_API_URL = "https://api.github.com/repos/microsoft/DirectXShaderCompiler/releases/latest"


def _get_project_dir():
    config_ini = _BUILD_DIR / "config.ini"
    if config_ini.exists():
        try:
            from ..core.config import get_cfg
            return Path(get_cfg().project_dir)
        except Exception:
            pass
    return _BUILD_DIR.parent

PROJECT_DIR = _get_project_dir()
VENDOR_DIR  = PROJECT_DIR / "engine" / "vendor" / "dxc"


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

def preflight():
    require_tool("git")
    require_tool("curl")

def _fetch_latest_release():
    print("Fetching latest DXC release from GitHub ...")
    result = subprocess.run(
        ["curl", "-fsSL", "-H", "Accept: application/vnd.github+json", GITHUB_API_URL],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        sys.exit(f"curl failed: {result.stderr.strip()}")
    return json.loads(result.stdout)


# ---------------------------------------------------------------------------
# macOS: headers + dylib from the LunarG Vulkan SDK
# ---------------------------------------------------------------------------

def _find_vulkan_sdk():
    sdk_env = os.environ.get("VULKAN_SDK")
    if sdk_env:
        p = Path(sdk_env)
        if (p / "lib" / "libdxcompiler.dylib").exists():
            return p

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
    win_zip = next(
        (a for a in assets
         if a["name"].endswith(".zip") and not a["name"].startswith("pdb")),
        None,
    )
    if win_zip is None:
        print("  No Windows zip asset found to extract headers from.")
        return False

    tmp = PROJECT_DIR / win_zip["name"]
    download_file(win_zip["browser_download_url"], tmp)

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
            "Then re-run:      make setup --update"
        )
        return False

    print(f"Vulkan SDK found: {sdk}")

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
    download_file(asset["browser_download_url"], tmp)

    VENDOR_DIR.mkdir(parents=True, exist_ok=True)
    if asset["name"].endswith(".zip"):
        require_tool("unzip")
        run_command(["unzip", "-q", "-o", str(tmp), "-d", str(VENDOR_DIR)])
    else:
        run_command(["tar", "-xzf", str(tmp), "-C", str(VENDOR_DIR)])

    tmp.unlink()
    return True


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

    ensure_gitignored(
        PROJECT_DIR / ".gitignore",
        "engine/vendor/dxc/",
        comment="# DXC pre-built binaries (downloaded by setup/dxc.py)",
    )

    print(f"\nDXC installed at {VENDOR_DIR}")
    print("Layout:")
    print("  engine/vendor/dxc/include/dxc/dxcapi.h")
    print("  engine/vendor/dxc/include/dxc/WinAdapter.h")
    print("  engine/vendor/dxc/lib/libdxcompiler.dylib  (macOS)")
    print("  engine/vendor/dxc/lib/libdxcompiler.so     (Linux)")
    print("  engine/vendor/dxc/lib/dxcompiler.dll       (Windows)")
