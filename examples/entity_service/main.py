import os
import subprocess
import sys
from pathlib import Path

if sys.platform.startswith("win"):
    print("Sorry, no idea of how to run services on Windows...")
    sys.exit(44)

# It seems that we cannot start a docker in docker on GitHub.
# For now, don't run this example as a test
sys.exit(44)


thisdir = Path(__file__).resolve().parent
env = {var: os.environ[var] for var in ["PATH", "USER"]}

subprocess.check_call(["/bin/sh", f"{thisdir}/start_service.sh"], env=env)
subprocess.check_call([sys.executable, f"{thisdir}/populate_database.py"])
subprocess.check_call([sys.executable, f"{thisdir}/get_instance.py"])
# subprocess.check_call([sys.executable, f"{thisdir}/try_service.py"])

# subprocess.check_call(["sh", f"{thisdir}/stop_services.sh"])
