import os
import subprocess
import sys
from pathlib import Path


if sys.platform.startswith("win"):
    print("Sorry, no idea of how to run services on Windows...")
    sys.exit(44)


thisdir = Path(__file__).resolve().parent

subprocess.check_call(
    ["/bin/sh", f"{thisdir}/start_service.sh"],
    env={"PATH": os.environ["PATH"]},
)
subprocess.check_call([sys.executable, f"{thisdir}/populate_database.py"])
subprocess.check_call([sys.executable, f"{thisdir}/get_instance.py"])
#subprocess.check_call([sys.executable, f"{thisdir}/try_service.py"])

#subprocess.check_call(["sh", f"{thisdir}/stop_services.sh"])
