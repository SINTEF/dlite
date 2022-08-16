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

# Store
inst1.save("mongodb", "localhost", "user=testuser;password=testpw;mock=true")
