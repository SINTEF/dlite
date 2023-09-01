import sys
from pathlib import Path

import dlite


thisdir = Path(__file__).resolve().parent
exampledir = thisdir
plugindir = exampledir / "plugins"
entitydir = exampledir / "entities"
indir = exampledir / "data"
outdir = exampledir / "output"

filenames = "040.dm3", "6c8cm_008.dm3",  "BF_100-at-m5-and-2_001.dm3"
temdata = "https://folk.ntnu.no/friisj/temdata"

dlite.storage_path.append(entitydir)
dlite.python_storage_plugin_path.append(plugindir)

sys.path.append(str(exampledir))


# Hack - add lib64 to sys.path
#sys.path.insert(0, "/home/friisj/.envs/temdata/lib64/python3.11/site-packages")
