#!/usr/bin/env python3
"""Run clang-tidy or Clazy on project entries from a CMake compilation database."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CLAZY_CHECKS = "connect-non-signal,lambda-in-connect,lambda-unique-connection,wrong-qevent-cast,writing-to-temporary"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path)
    parser.add_argument("--tool", choices=("clang-tidy", "clazy"), default="clang-tidy")
    parser.add_argument("--executable", help="Explicit path to the analysis tool")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--checks", help="Override/adjust clang-tidy checks for a CI phase")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    build = args.build_dir.resolve()
    database = build / "compile_commands.json"
    if not database.is_file():
        parser.error("Configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON and build first")
    executable = args.executable
    if not executable:
        executable = (shutil.which("clang-tidy-18") or shutil.which("clang-tidy")) if args.tool == "clang-tidy" else shutil.which("clazy-standalone")
    if not executable:
        parser.error(f"Cannot find {args.tool}; supply --executable")
    sources = set()
    for entry in json.loads(database.read_text()):
        source = Path(entry["file"])
        if not source.is_absolute():
            source = Path(entry["directory"]) / source
        source = source.resolve()
        try:
            relative = source.relative_to(ROOT)
        except ValueError:
            continue
        if relative.parts[0] not in ("src", "tests") or source.suffix not in (".cpp", ".mm"):
            continue
        if args.tool == "clazy":
            # Ubuntu 24.04's Clazy uses LLVM 15. Analyze the Qt application
            # layer; leave pqxx/C++20 backend and test analysis to clang-tidy 18.
            if relative.parts[0] != "src" or relative.parts[:3] == ("src", "core", "connections"):
                continue
        sources.add(source)
    if not sources:
        parser.error("No project sources found in the compilation database")
    header_filter = re.escape(str(ROOT)) + r"/(src|tests)/"

    def analyze(source):
        command = [executable, f"-p={build}"]
        if args.tool == "clang-tidy":
            command += [f"--header-filter={header_filter}", "--quiet"]
            if args.checks:
                command += [f"--checks={args.checks}"]
        else:
            command += [f"-checks={CLAZY_CHECKS}", f"-header-filter={header_filter}",
                        f"-ignore-dirs={re.escape(str(ROOT))}/(vendor|build)/"]
        result = subprocess.run(command + [str(source)], cwd=ROOT, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        # clazy-standalone may return zero for warnings; these checks are gates.
        failed = result.returncode != 0 or (args.tool == "clazy" and re.search(r"\[(?:-W)?clazy-", result.stdout))
        return source, result.stdout, bool(failed)

    failures = 0
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for source, output, failed in pool.map(analyze, sorted(sources)):
            print(f"{'FAIL' if failed else 'OK'} {source.relative_to(ROOT)}", flush=True)
            if output.strip():
                print(output, flush=True)
            failures += failed
    return int(failures != 0)


if __name__ == "__main__":
    sys.exit(main())
