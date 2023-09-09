"""This script is called by the DLite test system.

Runs all tests as well as the demo itself.
"""
import sys
from subprocess import PIPE, call

from paths import exampledir, outdir, testdir


# The error code of each test
status = {}

# Run all tests
for test in testdir.glob("test_*.py"):
    print(test.name)
    status[test.stem] = call([sys.executable, test], stdout=PIPE)


# Remove files created by the demo from output directory
print("demo.py")
created_files = [
    "precipitate_statistics.csv",
    "precip.txt",
    "resources.ttl",
    "temsettings.json",
    "thumbnail.png",
]
for name in created_files:
    path = outdir / name
    if path.exists():
        path.unlink()

# Run demo
status["demo"] = call([sys.executable, exampledir / "demo.py"], stdout=PIPE)

# Check that expected output files have been created with a size larger
# than zero
for name in created_files:
    path = outdir / name
    if not path.exists():
        print("missing output file:", path.name)
        status["demo"] |= 1
    if path.stat().st_size == 0:
        print("empty output file:", path.name)
        status["demo"] |= 1


# Summary
print("=" * 79)
for name, code in status.items():
    stat = "passed" if code == 0 else "failed"
    print(f"{name:40}{stat}")


# Exit with non-zero error code in case of any failure
sys.exit(sum(status.values()))
