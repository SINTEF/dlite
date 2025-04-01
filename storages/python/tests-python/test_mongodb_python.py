"""Script to test the 'mongodb' DLite plugin from Python."""
import os
import sys
from pathlib import Path

import dlite
from dlite.options import Options
from dlite.testutils import importskip

importskip("pymongo", env_exitcode=None)
mongomock = importskip("mongomock", env_exitcode=None)

from dlite.testutils import importskip


@mongomock.patch(servers=(('localhost', 27017),))
def create_storage():
    """ Create storage with mongomock """
    storage = dlite.Storage(
        "mongodb", "localhost", "user=testuser;password=testpw"
    )
    return storage


if 'DLITETEST_MONGODB_AZURE' in os.environ:
    print('Test MongoDB on Azure')
    conn_str = os.environ['DLITETEST_MONGODB_AZURE']
    options = Options("mode=w")
    options.update(
        database=os.environ['DLITETEST_MONGODB_AZURE_DATABASE'],
        collection=os.environ['DLITETEST_MONGODB_AZURE_COLLECTION']
    )
    storage = dlite.Storage("mongodb", conn_str, str(options))
    print(storage.options)
    instances = list(storage.instances())
    print('azure instances', len(instances))
    assert len(instance) == int(os.environ['DLITETEST_MONGODB_AZURE_COUNT'])


thisdir = Path(__file__).absolute().parent
inputdir = thisdir / "input"

# Load existing test data
meta = dlite.Instance.from_location("json", inputdir / "test_meta.json")
inst1 = dlite.Instance.from_location("json", inputdir / "test_data.json",
                                     id="52522ba5-6bfe-4a64-992d-e9ec4080fbac")
inst2 = dlite.Instance.from_location("json", inputdir / "test_data.json",
                                     id="410ace1a-1e71-5e08-9ff3-b952307dbffe")


storage = create_storage()
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
