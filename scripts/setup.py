# setup.py
from setup_premake import main as setup_premake
import argparse

def main(update=False):
    setup_premake(update=update)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()
    main(update=args.update)