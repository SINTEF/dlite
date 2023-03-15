import sys
import os
from pathlib import Path
import dlite

# Load user and password from environment variables
user = os.environ.get("DLITETEST_MONGODB_USER")
password = os.environ.get("DLITETEST_MONGODB_PASSWORD")
server = os.environ.get("DLITETEST_MONGODB_SERVER")
inputdir = os.environ.get("DLITETEST_MONGODB_INPUTDIR")

# Check if any of the required environment variables are missing
if not user or not password or not server or not inputdir:
    sys.exit(44)  # skip test if any required environment variables are not provided

uri = f"mongodb+srv://{server}"
inputdir = Path(inputdir)

# Load existing test data
meta = dlite.Instance.from_location("json", inputdir / "test_meta.json")
inst1 = dlite.Instance.from_location("json", inputdir / "test_data.json",
                                     id="52522ba5-6bfe-4a64-992d-e9ec4080fbac")
inst2 = dlite.Instance.from_location("json", inputdir / "test_data.json",
                                     id="2f8ba28c-add6-5718-a03c-ea46961d6ca7")

# Create a storage instance and save test data
storage = dlite.Storage(
    'mongodb', uri,
    options=f'user={user};password={password}'
)
storage.save(inst1)
