#!/usr/bin/env python3
"""Require an actual sanitizer diagnostic from a deliberately faulty probe."""
import argparse
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("sanitizer", choices=("thread", "memory"))
parser.add_argument("probe")
args = parser.parse_args()
expected = {"thread": "ThreadSanitizer: data race",
            "memory": "MemorySanitizer: use-of-uninitialized-value"}[args.sanitizer]
result = subprocess.run([args.probe], text=True, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, timeout=30)
if result.returncode == 0 or expected not in result.stdout:
    print(result.stdout)
    print(f"Runtime verification failed: expected {expected!r} and a nonzero exit", file=sys.stderr)
    sys.exit(1)
print(f"Verified {expected}")
