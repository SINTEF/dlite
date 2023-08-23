"""Script to test the 'redis' DLite plugin from Python."""
import sys
from time import sleep

from pathlib import Path

import dlite

try:
    import redis
except ImportError:
    print("redis-py not installed, skipping test")
    sys.exit(44)  # skip test


thisdir = Path(__file__).resolve().parent
dlite.storage_path.append(thisdir / "input/*.json")


inst1 = dlite.get_instance("2f8ba28c-add6-5718-a03c-ea46961d6ca7")
inst2 = dlite.get_instance("52522ba5-6bfe-4a64-992d-e9ec4080fbac")
meta = inst1.meta

save0 = meta.asdict()
save1 = inst1.asdict()
save2 = inst2.asdict()


# Test storing to Redis
with dlite.Storage("redis", "localhost", "expire=1;db=13") as s:
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


# Test instance expiration
print("Wait for instances in redis storage has expired...")
sleep(1.1)

del newinst1
try:
    with dlite.silent:
        newinst1 = dlite.Instance.from_location(
            "redis", "localhost", "port=6379;db=13", id=save1["uuid"]
        )
except dlite.DLiteError:
    pass
else:
    raise RuntimeError("expected redis storage to have expired")


# Test encryption
try:
    from cryptography.fernet import Fernet
except ImportError:
    print("Skipping testing encryption")
else:
    fernet_key = Fernet.generate_key().decode()
    options = f"port=6379;expire=1;db=13;fernet_key={fernet_key}"

    newinst2.save("redis", "localhost", options)

    del newinst2
    inst2 = dlite.Instance.from_location(
        "redis", "localhost", options, id=save2["uuid"],
    )
