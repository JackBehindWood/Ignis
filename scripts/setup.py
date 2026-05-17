# setup.py
import argparse
from setup_premake import main as setup_premake
from setup_dxc     import main as setup_dxc

def main(update=False):
    setup_premake(update=update)
    setup_dxc(update=update)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()
    main(update=args.update)