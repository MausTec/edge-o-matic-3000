import subprocess

revision = (
    subprocess.check_output(["git", "describe", "--tags", "--abbrev=1"])
    .strip()
    .decode("utf-8")
)

branch = (
    subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"])
    .strip()
    .decode("utf-8")
)

if branch != "main":
    print("-DVERSION='\"%s/%s\"'" % (branch, revision))
else:
    print("-DVERSION='\"%s\"'" % revision)
