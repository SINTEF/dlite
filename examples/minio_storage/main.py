import os
import sys
import subprocess
from pathlib import Path

from dlite.testutils import importskip, serverskip


importskip("minio")  # skip this test if minio is not available
#serverskip("play.min.io", 9000)  # skip test if minio is down

thisdir = Path(__file__).resolve().parent

# Get timeout from the environment
timeout_str = os.getenv("TIMEOUT", None)
timeout = float(timeout_str) if timeout_str else None

subprocess.check_call([sys.executable, f"{thisdir}/store.py"], timeout=timeout)
subprocess.check_call([sys.executable, f"{thisdir}/fetch.py"], timeout=timeout)
print(f"=== timeout={timeout} ===")
