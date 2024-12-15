# This test assumes that test_collection.py has been run first
import json
from pathlib import Path

import dlite
from dlite.utils import instance_from_dict


thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"
outdir = thisdir / "output"
dlite.storage_path.append(thisdir)
dlite.storage_path.append(thisdir / "entities")


# -- Test for PR #750

# check initialisation of collection
Coll = dlite.get_instance(dlite.COLLECTION_ENTITY)
assert Coll["dimensions"].size

# Check that we can load collection from dict
d = json.load(open(indir / "coll.json", "r"))
with dlite.HideDLiteWarnings():
    coll = instance_from_dict(d)
assert coll.nrelations == 6
