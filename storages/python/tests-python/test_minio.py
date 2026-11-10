"""Script to test the 'minio' DLite plugin from Python."""
import sys

from pathlib import Path

import dlite
from dlite.testutils import importskip, serviceskip


# skip this test if minio or urllib3 are not available
importskip("minio")
importskip("urllib3")

# Skip test if the MinIO server is down.  The server and its credentials
# are configured in examples/docker-compose.yml (see
# dlite.testutils.service_config()).
config = serviceskip("minio")
location = f"{config['host']}:{config['port']}"


thisdir = Path(__file__).resolve().parent
dlite.storage_path.append(thisdir / "input/*.json")


options = (
    f"bucket_name=dlite-test;access_key={config['access_key']};"
    f"secret_key={config['secret_key']};secure=false;timeout=2"
)

inst1 = dlite.get_instance("http://data.org/my_test_instance_1")
inst2 = dlite.get_instance("52522ba5-6bfe-4a64-992d-e9ec4080fbac")
meta = inst1.meta


save0 = meta.asdict()
save1 = inst1.asdict()
save2 = inst2.asdict()

uuid0 = meta.uuid
uuid1 = inst1.uuid
uuid2 = inst2.uuid


# Empty minio store
with dlite.Storage("minio", location, options=options) as s:
    uuids = list(s.get_uuids())
    for uuid in uuids:
        s.delete(uuid)


# Test storing
with dlite.Storage("minio", location, options=options) as s:
    try:
        s.save(inst1)
    except dlite.DLiteError as exc:
        if "ConnectionError:" in str(exc):
            print(
                f"cannot connect to MinIO server: {exc}\n\n"
                "Is the MinIO server running?"
            )
            sys.exit(44)
        raise
    s.save(inst2)
    s.save(inst1.meta)  # Save metadata as well


# Delete instances
meta._decref()
del meta
del inst1
del inst2


# Test retrieving
with dlite.Storage("minio", location, options=options) as s:
    newmeta = s.load(uuid0)
    newinst1 = s.load(uuid1)
    newinst2 = s.load(uuid2)

    assert newmeta.asdict() == save0
    assert newinst1.asdict() == save1
    assert newinst2.asdict() == save2

    assert set(s.get_uuids()) == set([uuid0, uuid1, uuid2])


del newinst2
inst = dlite.Instance.from_location(
    "minio", location, options=options, id=uuid2,
)
assert inst.asdict() == save2


# Test remove
with dlite.Storage("minio", location, options=options) as s:
    s.delete(uuid2)
    s.delete(uuid1)
    s.delete(uuid0)
    s.flush()
    assert s.get_uuids() == []
