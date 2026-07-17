"""Script to test the 'minio' DLite plugin from Python."""
import subprocess
import sys
from pathlib import Path

import dlite
from dlite.testutils import importskip, Service


# skip this test if minio or urllib3 are not available
importskip("minio")
importskip("urllib3")


thisdir = Path(__file__).resolve().parent
rootdir = thisdir.parent.parent.parent
dlite.storage_path.append(thisdir / "input/*.json")

# Start test server
host = "localhost:9000"
access_key = "minioadmin"
secret_key = "minioadmin"
secure = False
options = (
    f"bucket_name=dlite-test;access_key={access_key};"
    f"secret_key={secret_key};secure={secure};timeout=2"
)
serv = Service("minio")
serv.start()
assert serv.status()

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
with dlite.Storage("minio", host, options=options) as s:
    uuids = list(s.get_uuids())
    for uuid in uuids:
        s.delete(uuid)


# Test storing
with dlite.Storage("minio", host, options=options) as s:
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
with dlite.Storage("minio", host, options=options) as s:
    newmeta = s.load(uuid0)
    newinst1 = s.load(uuid1)
    newinst2 = s.load(uuid2)

    assert newmeta.asdict() == save0
    assert newinst1.asdict() == save1
    assert newinst2.asdict() == save2

    assert set(s.get_uuids()) == set([uuid0, uuid1, uuid2])


del newinst2
inst = dlite.Instance.from_location(
    "minio", host, options=options, id=uuid2,
)
assert inst.asdict() == save2


# Test remove
with dlite.Storage("minio", host, options=options) as s:
    s.delete(uuid2)
    s.delete(uuid1)
    s.delete(uuid0)
    s.flush()
    assert s.get_uuids() == []
