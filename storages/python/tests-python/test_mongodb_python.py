"""Script to test the 'mongodb' DLite plugin from Python."""
import os
import sys
from pathlib import Path

import dlite
from dlite.options import Options
from dlite.testutils import importskip, service_available, service_config

importskip("pymongo", env_exitcode=None)


def create_storage():
    """Create storage.

    Uses the MongoDB server configured in examples/docker-compose.yml
    (see dlite.testutils.service_config()) if it is running.  Otherwise
    the server is mocked with mongomock.
    """
    if service_available("mongodb"):
        print("Test with MongoDB server")
        config = service_config("mongodb")
        storage = dlite.Storage(
            "mongodb",
            f"mongodb://{config['host']}:{config['port']}",
            f"mode=w;database=dlite_test;collection=test_coll;"
            f"username={config['user']};password={config['password']}",
        )
        # Start with an empty collection.  Fall back to mongomock if
        # the server doesn't accept the test credentials (e.g. if
        # another MongoDB server is running on the same port).
        try:
            for uuid in storage.get_uuids():
                storage.delete(uuid)
        except dlite.DLiteError as exc:
            if "Authentication failed" not in str(exc):
                raise
            print(f"MongoDB server doesn't accept test credentials: {exc}")
        else:
            return storage

    print("Test with mongomock")
    mongomock = importskip("mongomock", env_exitcode=None)

    @mongomock.patch(servers=(('localhost', 27017),))
    def create_mock_storage():
        return dlite.Storage(
            "mongodb", "localhost", "mode=w;user=testuser;password=testpw"
        )

    return create_mock_storage()


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
# Don't keep a reference to the iterator, since it keeps the storage alive
assert set(storage.instances(meta.uri)) == set([new_inst1, new_inst2])

uuids = storage.get_uuids()
assert len(uuids) == 3
meta_uuid, = storage.get_uuids(dlite.ENTITY_SCHEMA)
assert meta_uuid == uuid0
inst_uuids = storage.get_uuids(meta.uri)
assert set(inst_uuids) == set([uuid1, uuid2])

# Clean up
for uuid in storage.get_uuids():
    storage.delete(uuid)
assert not storage.get_uuids()

# Close the storage before Python shuts down.  If it is first closed
# during shutdown, the plugin's close() method fails and dlite may crash.
del storage
