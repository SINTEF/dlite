import sys
import subprocess
from pathlib import Path

try:
    import minio  # noqa: F401
except ImportError:
    import sys
    sys.exit(44)  # skip this test if minio is not available


thisdir = Path(__file__).resolve().parent

subprocess.check_call(args=[sys.executable, f"{thisdir}/store.py"])
subprocess.check_call(args=[sys.executable, f"{thisdir}/fetch.py"])
