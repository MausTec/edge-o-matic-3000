#!/usr/bin/env python3
"""
Prepares compile_commands.json for qodana-clang.

Reads the PlatformIO-generated compile_commands.json, filters out all entries
that are not part of this project (i.e. excludes .platformio framework files),
and rewrites host-absolute paths to /data/project for Docker compatibility.

Usage: python3 bin/qodana_prep.py
Output: build/compile_commands.json
"""

import json
import os
import sys

SOURCE = "compile_commands.json"
DEST = "build/compile_commands.json"
DOCKER_ROOT = "/data/project"

with open(SOURCE) as f:
    entries = json.load(f)

# Derive host_root from the data: the shortest absolute `directory` value
# is always the project root on the host (subdirs like .pio/build/... sort after it).
abs_dirs = sorted(d for e in entries if os.path.isabs(d := e.get("directory", "")))
if not abs_dirs:
    print("ERROR: no absolute directory entries found in compile_commands.json", file=sys.stderr)
    sys.exit(1)
host_root = abs_dirs[0]

filtered = [
    e for e in entries
    if os.path.abspath(os.path.join(e.get("directory", ""), e["file"])).startswith(host_root)
]

if not filtered:
    print(f"ERROR: No project files found in {SOURCE} under {host_root}", file=sys.stderr)
    sys.exit(1)

def rewrite(s):
    return s.replace(host_root, DOCKER_ROOT)

out = [
    {
        "command": rewrite(e["command"]),
        "directory": rewrite(e["directory"]),
        "file": rewrite(e["file"]),
    }
    for e in filtered
]

os.makedirs("build", exist_ok=True)
with open(DEST, "w") as f:
    json.dump(out, f, indent=2)

print(f"Written {len(out)} entries to {DEST}")
