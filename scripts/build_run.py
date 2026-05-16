from build import main as build_project
from run import main as run_project

import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--dev", action="store_true", default=None)
    parser.add_argument("--no-dev", dest="dev", action="store_false")
    parser.add_argument("--config", choices=["Debug", "Release", "Distribution"], default=None)
    parser.add_argument("--verbose", action="store_true", default=None)
    args = parser.parse_args()
    build_project(clean=args.clean, dev=args.dev, configuration=args.config, verbose=args.verbose)
    run_project(configuration=args.config)

if __name__ == "__main__":
    main()