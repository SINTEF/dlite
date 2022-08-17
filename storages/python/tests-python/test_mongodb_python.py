"""Script to test the 'mongodb' DLite plugin from Python."""
import sys
from pathlib import Path

import dlite

try:
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
s = dlite.Storage(
    "mongodb", "localhost", "user=testuser;password=testpw;mock=true"
)

# Store metadata and data
meta.save(s)
inst1.save(s)
inst2.save(s)

# Remove existing instances
uuid0 = meta.uuid
uuid1 = inst1.uuid
uuid2 = inst2.uuid
del inst1
del inst2
del meta

# Load metadata and data
new_meta = s.load(id=uuid0)
new_inst1 = s.load(id=uuid1)
new_inst2 = s.load(id=uuid2)

# Simple queries
instances = list(s.instances())
assert len(instances) == 3
assert new_meta in instances
assert new_inst1 in instances
assert new_inst2 in instances

uuids = s.get_uuids()
assert len(uuids) == 3
