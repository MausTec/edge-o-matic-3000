import subprocess

try:
    revision = (
        subprocess.check_output(["git", "describe", "--tags", "--abbrev=1"], stderr=subprocess.STDOUT)
        .strip()
        .decode("utf-8")
    )
except subprocess.CalledProcessError as err:
    print("Error: " + err.output, file=sys.stderr)
    pass

try:
    branch = (
        subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], stderr=subprocess.STDOUT)
        .strip()
        .decode("utf-8")
    )
except subprocess.CalledProcessError as err:
    print("Error: " + err.output, file=sys.stderr)
    pass

if branch != "main":
    print("-DEOM_FW_VERSION='\"%s/%s\"'" % (branch, revision))
else:
    print("-DEOM_FW_VERSION='\"%s\"'" % revision)
