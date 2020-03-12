#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


def main(args):
    gdbinit_common_dir = os.path.join(SCRIPT_DIR, "gdbinit-common")
    elf_dir = os.path.join(SCRIPT_DIR, "..", "..", "bin", "laritos.elf")

    cmd = "gdb-multiarch -q {} -nx -ex \"source {}\" -ex \"source {}\"".format(elf_dir, gdbinit_common_dir, args.board_gdbinit)
    os.system(cmd)


def parse_args(argv):
    parser = argparse.ArgumentParser(description="laritOS debugger",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-g", "--board-gdbinit", default=os.path.join(SCRIPT_DIR, "gdbinit-qemu"),
                       help="board-specific gdbinit file to use")
    return parser.parse_args(argv)


if __name__ == "__main__":
    try:
        args = parse_args(sys.argv[1:])
        main(args)
        sys.exit(0)
    except Exception as e:
        print("Error: {}".format(str(e)))
        sys.exit(1)
