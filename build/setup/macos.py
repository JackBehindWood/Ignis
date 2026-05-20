import os
import subprocess
import sys

from ..core.utils import run_command, prompt_bool

_XCODE_DEV = "/Applications/Xcode.app/Contents/Developer"


def _metal_path():
    result = subprocess.run(["xcrun", "-find", "metal"], capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else None


def _metal_toolchain_ok():
    result = subprocess.run(["xcrun", "metal", "--version"], capture_output=True, text=True)
    return result.returncode == 0 and "missing Metal Toolchain" not in (result.stdout + result.stderr)


def _clt_installed():
    return subprocess.run(["xcode-select", "-p"], capture_output=True).returncode == 0


def main(**_):
    if path := _metal_path():
        if _metal_toolchain_ok():
            print(f"xcrun metal: ok  ({path})")
            return

        print("Metal stub found but toolchain missing — downloading Metal Toolchain ...")
        run_command(["xcodebuild", "-downloadComponent", "MetalToolchain"])
        return

    if _clt_installed():
        print("Xcode Command Line Tools installed but metal compiler not found.")

        if os.path.isdir(_XCODE_DEV):
            print(f"Xcode.app found — switching xcode-select to {_XCODE_DEV}")
            run_command(["sudo", "xcode-select", "-s", _XCODE_DEV])
            run_command(["sudo", "xcodebuild", "-license", "accept"])
        else:
            print("Running softwareupdate to install missing components ...")
            run_command(["softwareupdate", "--install", "--all", "--agree-to-license"])
        return

    print("Xcode Command Line Tools not found.")
    if not prompt_bool("Install now?", default=True):
        sys.exit(1)

    run_command(["xcode-select", "--install"])


def main_from_args(args):
    main(update=args.update)
