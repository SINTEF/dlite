import sys
import os
from pathlib import Path

import dlite
from dlite.testutils import importskip

importskip("pymongo", env_exitcode=None)


# Get the current file path
current_file = Path(__file__).resolve()

# Define the credentials file path in the root directory
credentials_file = current_file.parent.parent.parent.parent / "mongodb-atlas_credentials.txt"


# If the credentials file exists, load user and password from the file
# The credentials.txt file should have the following structure:
# DLITETEST_MONGODB_USER
# DLITETEST_MONGODB_PASSWORD
# DLITETEST_MONGODB_SERVER
if credentials_file.exists():
    with open(credentials_file, "r") as f:
        user, password, server = [line.strip() for line in f.readlines()]

else:
    # Load user, password, server, and inputdir from environment variables
    user = os.environ.get("DLITETEST_MONGODB_USER")
    password = os.environ.get("DLITETEST_MONGODB_PASSWORD")
    server = os.environ.get("DLITETEST_MONGODB_SERVER")

# Check if any of the required variables are missing
if not user or not password or not server:
    sys.exit(44)  # skip test if any required variables are not provided

uri = f"mongodb+srv://{user}:{password}@{server}"

# Set inputdir as a hardcoded relative path
inputdir = current_file.parent / "../../python/tests-python/input"
inputdir = inputdir.resolve()
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
