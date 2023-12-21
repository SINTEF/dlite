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
coll = instance_from_dict(d)
assert coll.nrelations == 6


# -- Test loading collection
#collfile = outdir / "coll0.json"
#
## Alt. 1: If we know that we only have one collection in the storage
#with dlite.Storage("json", collfile, "mode=r") as s:
#    generator = s.instances(dlite.COLLECTION_ENTITY)
#    coll2 = generator.next()
#    if generator.next():
#        raise dlite.DLiteStorageLoadError(
#            "Storage '{collfile}' contain more than one collection. "
#            "`id` is needed to select which one to load"
#        )
#assert coll2.nrelations == 8
#
## Alt. 2: Make a list of all collections in the storage
#with dlite.Storage("json", collfile, "mode=r") as s:
#    collections = list(s.instances(dlite.COLLECTION_ENTITY))
#assert len(collections) == 1
#assert collections[0].nrelations == 8
