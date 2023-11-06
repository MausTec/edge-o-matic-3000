import subprocess
import sys

revision = ""
branch = ""

try:
    revision = (
        subprocess.check_output(["git", "describe", "--tags", "--abbrev=1"], stderr=subprocess.STDOUT)
        .strip()
        .decode("utf-8")
    )
except subprocess.CalledProcessError as err:
    print("Error: " + err.output.decode("utf-8"), file=sys.stderr)
    exit(1)

try:
    branch = (
        subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], stderr=subprocess.STDOUT)
        .strip()
        .decode("utf-8")
    )
except subprocess.CalledProcessError as err:
    print("Error: " + err.output.decode("utf-8"), file=sys.stderr)
    exit(1)

if branch != "main" and branch != "master":
    print("-DEOM_FW_VERSION='\"%s+%s\"'" % (revision, branch))
else:
    print("-DEOM_FW_VERSION='\"%s\"'" % revision)
