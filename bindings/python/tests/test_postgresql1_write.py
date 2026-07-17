import sys
import re
import socket
from pathlib import Path

import dlite
from dlite.testutils import importskip, Service


# Skip this test if psycopg is not available
importskip("psycopg")

# Paths
thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"
outdir = thisdir / "output"
entitydir = thisdir / "entities"

# Start test server
host = "localhost"
options = "database=testdb;user=pguser;password=pgpass"
serv = Service("postgres", autostop=False)
serv.start()
assert serv.status()

# Add metadata to search path
dlite.storage_path.append(f"{entitydir}/Person.json")
dlite.storage_path.append(f"{entitydir}/SimplePerson.json")

# Load dataset
inst = dlite.Instance.from_location(
    "json",
    f"{indir}/persons.json",
    id="Cleopatra",
)

# Save to postgresql DB
inst.save(f"postgresql://{host}?{options}")
