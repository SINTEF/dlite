"""Script to test the DLite plugin 'yaml.py' in Python."""
from pathlib import Path

import dlite
from dlite.testutils import importskip

yaml = importskip("yaml")


thisdir = Path(__file__).resolve().parent
indir = thisdir / 'input'
outdir = thisdir / 'output'


# Test loading metadata
meta = dlite.Instance.from_location("yaml", indir / "test_meta.yaml")
assert meta

# Test saveng metadata
meta.save("yaml", outdir / "test_meta.yaml", options="mode=w;with_meta=true")

# Check that output matches input
with open(indir / "test_meta.yaml", "rt") as f:
    d1 = yaml.safe_load(f)
with open(outdir / "test_meta.yaml", "rt") as f:
    d2 = yaml.safe_load(f)
assert d1 == d2


# Test loading data
inst1 = dlite.Instance.from_location(
    "yaml", indir / "test_data.yaml", id="52522ba5-6bfe-4a64-992d-e9ec4080fbac"
)
inst2 = dlite.Instance.from_location(
    "yaml", indir / "test_data.yaml", id="http://data.org/my_test_instance_1"
)
assert inst1
assert inst2

# Test saveng data
with dlite.Storage("yaml", outdir / "test_data.yaml", options="mode=w") as s:
    s.save(inst1)
    s.save(inst2)

# Check that output matches input
with open(indir / "test_data.yaml", "rt") as f:
    d1 = yaml.safe_load(f)
with open(outdir / "test_data.yaml", "rt") as f:
    d2 = yaml.safe_load(f)
assert d1 == d2
