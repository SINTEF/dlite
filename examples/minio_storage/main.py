import os
import sys
import subprocess
import time
from pathlib import Path

from dlite.testutils import importskip, serverskip


importskip("minio")  # skip this test if minio is not available
#serverskip("play.min.io", 9000)  # skip test if minio is down

thisdir = Path(__file__).resolve().parent

# Get timeout from the environment
timeout_str = os.getenv("TIMEOUT", None)
timeout = float(timeout_str) if timeout_str else None

# The MinIO server is public and there can be a race condition
attempts = 0
while attempts < 3:
    try:
        subprocess.check_call([sys.executable, f"{thisdir}/store.py"], timeout=timeout)
        subprocess.check_call([sys.executable, f"{thisdir}/fetch.py"], timeout=timeout)
    except dlite.DLiteMissingInstanceError:
        attempts += 1
        print(f"Sleeping for 5s as failed to access expected instances in attempt {attempts} of 3")
        time.sleep(5)
    finally:
        break
