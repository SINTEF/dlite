import sys
from pathlib import Path

import requests

import dlite


thisdir = Path(__file__).resolve().parent
exampledir = thisdir.parent
plugindir = exampledir / "plugins"
entitydir = exampledir / "entities"
indir = exampledir / "data"
outdir = exampledir / "output"

filenames = "040.dm3", "6c8cm_008.dm3",  "BF_100-at-m5-and-2_001.dm3"
temdata = "https://folk.ntnu.no/friisj/temdata/"

dlite.storage_path.append(entitydir)
dlite.python_storage_plugin_path.append(plugindir)

sys.path.append(str(exampledir))


# Download TEM files from temdata website
for filename in filenames:
    temfile = outdir / filename
    if not temfile.exists():
        r = requests.get(temdata + filename)
        with open(temfile, "wb") as f:
            f.write(r.content)
