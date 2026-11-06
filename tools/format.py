#!/usr/bin/env python3
"""Format project C++ sources, or check them using the CI formatter version."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Report differences without editing files")
    parser.add_argument("--clang-format", default=os.environ.get("CLANG_FORMAT"))
    args = parser.parse_args()
    executable = args.clang_format or shutil.which("clang-format-18") or shutil.which("clang-format")
    if not executable:
        parser.error("Install clang-format 18 or set CLANG_FORMAT to its executable")
    version = subprocess.check_output([executable, "--version"], text=True)
    if not re.search(r"version 18\.", version):
        parser.error(f"CI uses clang-format 18; selected: {version.strip()}")
    sources = sorted(path for folder in ("src", "tests") for path in (ROOT / folder).rglob("*")
                     if path.suffix in (".cpp", ".h", ".mm"))
    command = [executable, "--style=file", "--fallback-style=none"]
    command += ["--dry-run", "--Werror"] if args.check else ["-i"]
    return subprocess.call(command + [str(path.relative_to(ROOT)) for path in sources], cwd=ROOT)


if __name__ == "__main__":
    sys.exit(main())
