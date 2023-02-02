"""Script to test the 'mongodb' DLite plugin from Python."""
import sys
import os
from pathlib import Path
import dlite


#TODO: handle the password in a way suitable for production
user_password_file = "/home/daniel/RnD/EMMC/mongodb/mongodb_atlas/password"
try: 
    with open(user_password_file, 'r') as f:
        user_password = f.readlines()
        user_password = [x.strip() for x in user_password]
except FileNotFoundError:
    sys.exit(44)  # skip test

user = user_password[0]
password = user_password[1]
#TODO: need to think a bit more about testing server
server = "softcluster.4wryr.mongodb.net/test"
uri = f"mongodb+srv://{server}"

CONNECTION_STRING = "mongodb+srv://{}:{}@{}".format(user, password, server)

from pathlib import Path

inputdir = Path("/home/daniel/RnD/EMMC/mongodb/dlite/storages/python/tests-python/input")

# Load existing test data
meta = dlite.Instance.from_location("json", inputdir / "test_meta.json")
inst1 = dlite.Instance.from_location("json", inputdir / "test_data.json",
                                     id="52522ba5-6bfe-4a64-992d-e9ec4080fbac")
inst2 = dlite.Instance.from_location("json", inputdir / "test_data.json",
                                     id="2f8ba28c-add6-5718-a03c-ea46961d6ca7")


storage = dlite.Storage(
    'mongodb', uri,
    options=f'user={user};password={password}'
)
storage.save(inst1)
