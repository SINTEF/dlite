"""Run example in "anotherpackage"."""
import subprocess
import sys
from pathlib import Path


thisdir = Path(__file__).resolve().parent

# Install mypackage - registers entry points
subprocess.check_call(
    [sys.executable, "-m", "pip", "install", "."],
    cwd=thisdir,
)

# Run script that uses the entry points
subprocess.check_call(
    [sys.executable, str(Path("anotherpackage") / "main.py")],
    cwd=thisdir,
)
