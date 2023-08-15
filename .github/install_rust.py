# A python script for installing rust in a Linux environment
import subprocess
import sys


if sys.platform.startswith("linux"):
    subprocess.run(
        "sudo apt install -y rustc cargo && python -m pip install -U poetry",
        check=True, shell=True,
    )
