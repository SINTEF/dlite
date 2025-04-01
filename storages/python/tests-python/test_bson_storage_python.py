"""Script to test the DLite plugin 'bson.py' in Python."""
from pathlib import Path

import dlite
from dlite.testutils import importskip

bson = importskip("bson")

thisdir = Path(__file__).resolve().parent
indir = thisdir / 'input'
outdir = thisdir / 'output'


# Test loading metadata
meta = dlite.Instance.from_location(
    "bson", indir / "test_meta.bson",
    id="d9910bde-6028-524c-9e0f-e8f0db734bc8",
)
assert meta

# Test saveng metadata
meta.save("bson", outdir / "test_meta.bson", options="mode=w")

# Check that output matches input
with open(indir / "test_meta.bson", "rb") as f:
    d1 = bson.decode(f.read())
with open(outdir / "test_meta.bson", "rb") as f:
    d2 = bson.decode(f.read())
assert d1 == d2


# Test loading data
with dlite.Storage("bson", indir / "test_data.bson") as s:
    inst1 = s.load(id="52522ba5-6bfe-4a64-992d-e9ec4080fbac")
    inst2 = s.load(id="410ace1a-1e71-5e08-9ff3-b952307dbffe")
    #inst2 = s.load(id="http://data.org/my_test_instance_1")
assert inst1
assert inst2


# Test saveng data
with dlite.Storage("bson", outdir / "test_data.bson", options="mode=w") as s:
    s.save(inst1)
    s.save(inst2)

# Check that output matches input
with open(indir / "test_data.bson", "rb") as f:
    d1 = bson.decode(f.read())
with open(outdir / "test_data.bson", "rb") as f:
    d2 = bson.decode(f.read())
assert d1 == d2
