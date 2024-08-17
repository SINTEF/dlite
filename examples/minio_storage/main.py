import sys
import subprocess
from pathlib import Path

from dlite.testutils import importskip, serverskip


importskip("minio")  # skip this test if minio is not available
#serverskip("play.min.io", 9000)  # skip test if minio is down

thisdir = Path(__file__).resolve().parent

subprocess.check_call(args=[sys.executable, f"{thisdir}/store.py"])
subprocess.check_call(args=[sys.executable, f"{thisdir}/fetch.py"])
