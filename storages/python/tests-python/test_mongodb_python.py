"""Script to test the 'mongodb' DLite plugin from Python."""
import sys
from pathlib import Path

import dlite

try:
    import pymongo
    import mongomock
except ImportError:
    sys.exit(44)  # skip test


thisdir = Path(__file__).absolute().parent
inputdir = thisdir / "input"

# Load existing test data
meta = dlite.Instance.from_location("json", inputdir / "test_meta.json")
inst1 = dlite.Instance.from_location("json", inputdir / "test_data.json",
                                     id="52522ba5-6bfe-4a64-992d-e9ec4080fbac")
inst2 = dlite.Instance.from_location("json", inputdir / "test_data.json",
                                     id="2f8ba28c-add6-5718-a03c-ea46961d6ca7")

# Create storage
storage = dlite.Storage(
    "mongodb", "localhost", "user=testuser;password=testpw;mock=true"
)

# Store metadata and data
meta.save(storage)
inst1.save(storage)
inst2.save(storage)

# Remove existing instances from dlite memory cache to ensure that we
# we are reading them from storage and not just from the cache...
# (This isn't sufficient for the metadata though...)
uuid0 = meta.uuid
uuid1 = inst1.uuid
uuid2 = inst2.uuid
del inst1
del inst2
del meta

# Load metadata and data
new_meta = storage.load(id=uuid0)
new_inst1 = storage.load(id=uuid1)
new_inst2 = storage.load(id=uuid2)

# Simple queries
instances = list(storage.instances())
assert len(instances) == 3
assert new_meta in instances
assert new_inst1 in instances
assert new_inst2 in instances
meta, = storage.instances(dlite.ENTITY_SCHEMA)
assert meta.uuid == uuid0
insts = storage.instances(meta.uri)
assert set(insts) == set([new_inst1, new_inst2])

uuids = storage.get_uuids()
assert len(uuids) == 3
meta_uuid, = storage.get_uuids(dlite.ENTITY_SCHEMA)
assert meta_uuid == uuid0
inst_uuids = storage.get_uuids(meta.uri)
assert set(inst_uuids) == set([uuid1, uuid2])
