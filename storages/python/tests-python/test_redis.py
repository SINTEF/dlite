"""Script to test the 'redis' DLite plugin from Python."""
import subprocess
import sys
from time import sleep

from pathlib import Path

import dlite
from dlite.testutils import importskip, Service

importskip("redis", env_exitcode=None)  # skip test if redis isn't installed

thisdir = Path(__file__).resolve().parent
rootdir = thisdir.parent.parent.parent
dlite.storage_path.append(thisdir / "input/*.json")

# Start test server
serv = Service("redis")
serv.start()
assert serv.status()


inst1 = dlite.get_instance("410ace1a-1e71-5e08-9ff3-b952307dbffe")
inst2 = dlite.get_instance("52522ba5-6bfe-4a64-992d-e9ec4080fbac")
meta = inst1.meta

save0 = meta.asdict()
save1 = inst1.asdict(single=True, uuid=True)
save2 = inst2.asdict(single=True, uuid=True)


# Keys stored by the tests below expire after this many seconds, so that
# the redis server is cleaned up even if the test fails.  Keep it long
# enough that the instances don't expire before we read them back when
# the machine is busy (e.g. with `ctest -j`).
EXPIRE = 60

# Test storing to Redis
with dlite.Storage("redis", "localhost", f"expire={EXPIRE};db=13") as s:
    try:
        s.save(inst1)
    except dlite.DLiteError as exc:
        if "ConnectionError:" in str(exc):
            print(
                f"cannot connect to Redis server: {exc}\n\n"
                "Is the Redis server running?"
            )
            sys.exit(44)
        raise
    s.save(inst2)
    s.save(inst1.meta)  # Save metadata as well


# Test retrieving from Redis
del meta
del inst1
del inst2
with dlite.Storage("redis", "localhost", "port=6379;db=13") as s:
    newmeta = s.load(save0["uuid"])
    newinst1 = s.load(save1["uuid"])

newinst2 = dlite.Instance.from_location(
    "redis", "localhost", "port=6379;db=13", id=save2["uuid"]
)


# Test deleting from Redis
with dlite.Storage("redis", "localhost", "port=6379;db=13") as s:
    uuid = save1["uuid"]
    assert uuid in s.get_uuids()
    s.delete(save1["uuid"])
    assert uuid not in s.get_uuids()


# Test instance expiration
# Remove what is left from the tests above (and earlier runs), then store
# an instance with a short expiration time and check that it disappears.
with dlite.Storage("redis", "localhost", "port=6379;db=13") as s:
    for uuid in s.get_uuids():
        s.delete(uuid)
    assert not s.get_uuids()
newinst1.save("redis", "localhost", "port=6379;db=13;expire=1")
print("Wait for instances in redis storage has expired...")
sleep(2)
with dlite.Storage("redis", "localhost", "port=6379;db=13") as s:
    assert not s.get_uuids()


# Test encryption
try:
    from cryptography.fernet import Fernet
except ImportError:
    print("Skipping testing encryption")
else:
    fernet_key = Fernet.generate_key().decode()
    options = f"port=6379;expire={EXPIRE};db=13;fernet_key={fernet_key}"

    newinst2.save("redis", "localhost", options)

    del newinst2
    inst2 = dlite.Instance.from_location(
        "redis", "localhost", options, id=save2["uuid"],
    )

    # Clean up
    with dlite.Storage("redis", "localhost", options) as s:
        s.delete(save2["uuid"])
        assert not s.get_uuids()
