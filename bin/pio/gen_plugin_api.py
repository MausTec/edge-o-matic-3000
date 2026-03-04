"""
gen_plugin_api.py — PlatformIO post-build script
Generates plugin-api.json into the build directory.

Scans:
  src/actions/*.c  for mta_register_system_function("name", fn, "permission"|NULL)
  include/actions/index.h  for mta_register_event("name", "permission"|NULL)

Version is read from `git describe --tags`, matching the build_flags.py pattern.

Output: $BUILD_DIR/plugin-api.json
"""

import json
import os
import re
import subprocess
import sys

Import("env")  # noqa: F821  (PlatformIO injects this)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def git_version():
    try:
        version = (
            subprocess.check_output(
                ["git", "describe", "--tags", "--abbrev=5"],
                stderr=subprocess.STDOUT,
            )
            .strip()
            .decode("utf-8")
        )
        # Strip leading 'v' if present so output matches semver
        return version.lstrip("v")
    except subprocess.CalledProcessError as e:
        print(
            "[gen_plugin_api] WARNING: git describe failed: %s" % e.output.decode(),
            file=sys.stderr,
        )
        return "0.0.0"


def extract_defines(source):
    """Return a dict of {MACRO_NAME: "string_value"} from #define lines in source."""
    return dict(re.findall(r'#define\s+(\w+)\s+"([^"]+)"', source))


def resolve_permission(raw, defines):
    """
    Given the raw third argument token from a registration call, return the
    string permission or None.

      "syscfg:read"  -> "syscfg:read"
      NULL           -> None
      PERM_SYSCFG_READ -> resolved via defines dict -> "syscfg:read"
    """
    raw = raw.strip()
    if raw == "NULL":
        return None
    if raw.startswith('"') and raw.endswith('"'):
        return raw[1:-1]
    # Try to resolve as a macro
    if raw in defines:
        return defines[raw]
    print(
        "[gen_plugin_api] WARNING: unresolved permission token '%s'" % raw,
        file=sys.stderr,
    )
    return raw


# ---------------------------------------------------------------------------
# Scanners
# ---------------------------------------------------------------------------

FUNC_RE = re.compile(
    r'mta_register_system_function\s*\(\s*"([^"]+)"\s*,\s*\w+\s*,\s*(NULL|"[^"]*"|\w+)\s*\)'
)

EVENT_RE = re.compile(
    r'mta_register_event\s*\(\s*"([^"]+)"\s*,\s*(NULL|"[^"]*"|\w+)\s*\)'
)


def scan_functions(actions_dir):
    entries = []
    for fname in sorted(os.listdir(actions_dir)):
        if not fname.endswith(".c"):
            continue
        path = os.path.join(actions_dir, fname)
        with open(path, "r") as f:
            source = f.read()
        defines = extract_defines(source)
        for name, perm_raw in FUNC_RE.findall(source):
            entries.append({
                "name": name,
                "permission": resolve_permission(perm_raw, defines),
            })
    return entries


def scan_events(index_h_path):
    entries = []
    with open(index_h_path, "r") as f:
        source = f.read()
    defines = extract_defines(source)
    for name, perm_raw in EVENT_RE.findall(source):
        entries.append({
            "name": name,
            "permission": resolve_permission(perm_raw, defines),
        })
    return entries


# ---------------------------------------------------------------------------
# PlatformIO action
# ---------------------------------------------------------------------------

def generate_plugin_api(source, target, env):
    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")

    actions_dir = os.path.join(project_dir, "src", "actions")
    index_h = os.path.join(project_dir, "include", "actions", "index.h")
    output_path = os.path.join(build_dir, "plugin-api.json")

    version = git_version()

    functions = scan_functions(actions_dir)
    events = scan_events(index_h)

    payload = {
        "$schema": "https://schemas.maus-tec.com/plugin-api/v1.0.0/plugin-api.json",
        "product": "edge-o-matic-3000",
        "version": version,
        "functions": functions,
        "events": events,
    }

    os.makedirs(build_dir, exist_ok=True)
    with open(output_path, "w") as f:
        json.dump(payload, f, indent=2)
        f.write("\n")

    print("[gen_plugin_api] Wrote %s (%d functions, %d events, version %s)" % (
        output_path, len(functions), len(events), version
    ))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", generate_plugin_api)  # noqa: F821
