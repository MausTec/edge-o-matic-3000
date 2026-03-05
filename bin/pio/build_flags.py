import subprocess
import sys
import re


def git_cmd(args):
    try:
        return (
            subprocess.check_output(args, stderr=subprocess.STDOUT)
            .strip()
            .decode("utf-8")
        )
    except subprocess.CalledProcessError as err:
        print("Error: " + err.output.decode("utf-8"), file=sys.stderr)
        sys.exit(1)


def sanitize_meta(s):
    """Replace chars not valid in SemVer build metadata identifiers with dots."""
    s = re.sub(r'[^A-Za-z0-9-]', '.', s)
    s = re.sub(r'\.{2,}', '.', s)
    return s.strip('.')


# --long always produces <tag>-<n>-g<sha>, even when sitting on an exact tag (n=0).
revision = git_cmd(["git", "describe", "--tags", "--long", "--abbrev=8"])
branch = git_cmd(["git", "rev-parse", "--abbrev-ref", "HEAD"])

# Parse the three components from: <tag>-<n>-g<sha>
m = re.match(r'^(.*)-(\d+)-g([0-9a-f]+)$', revision)
if not m:
    print("Error: unexpected git describe output: %s" % revision, file=sys.stderr)
    sys.exit(1)

tag_full = m.group(1)   # e.g. "v1.3.0-GPL" or "v2.0.0-rc.1"
n = int(m.group(2))     # commits since tag (0 = exactly on the tag)
sha = m.group(3)        # abbreviated commit hash

# Strip the leading 'v'. The remainder is the SemVer base (may include prerelease
# identifiers from the tag, e.g. "1.3.0-rc.1"). Tag authors are responsible for
# ensuring the tag suffix is valid SemVer prerelease syntax; check before tagging.
base = tag_full.lstrip('v')

# Build metadata: include sha when off-tag, sanitized branch when off main/master.
meta_parts = []
if n > 0:
    meta_parts.append(sha)

on_main = branch in ("main", "master")
if not on_main:
    meta_parts.append(sanitize_meta(branch))

if meta_parts:
    version = "%s+%s" % (base, ".".join(meta_parts))
else:
    version = base

print("-DPROJECT_VER='\"%s\"'" % version)
