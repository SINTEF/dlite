import sys
from pathlib import Path

import dlite


thisdir = Path(__file__).resolve().parent
exampledir = thisdir
plugindir = exampledir / "plugins"
entitydir = exampledir / "entities"
indir = exampledir / "data"
outdir = exampledir / "output"
testdir = exampledir / "tests"

temdata = "https://folk.ntnu.no/friisj/temdata"

dlite.storage_path.append(entitydir)
dlite.python_storage_plugin_path.append(plugindir)

sys.path.append(str(exampledir))
