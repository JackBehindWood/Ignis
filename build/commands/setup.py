from ..setup.premake import main as setup_premake
from ..setup.dxc     import main as setup_dxc
from ..setup.macos import main as setup_macos


def add_args(parser):
    parser.add_argument("--update", action="store_true", help="Force reinstall at version in config.ini")


def main(update=False):
    setup_premake(update=update)
    setup_dxc(update=update)
    setup_macos(update=update)


def main_from_args(args):
    main(update=args.update)
