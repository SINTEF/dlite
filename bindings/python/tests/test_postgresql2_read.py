"""This test must be run after test_postgresql_write
"""
import sys
import re
from pathlib import Path

import dlite
from dlite.testutils import importskip, Service
from test_postgresql1_write import host, options


# Skip this test if psycopg is not available
importskip("psycopg")

# Paths
thisdir = Path(__file__).resolve().parent

# Check if postgresql server is running
serv = Service("postgres")
assert serv.status()

# Add metadata to search path
dlite.storage_path.append(f"{thisdir}/Person.json")


# Read from postgresql DB
inst = dlite.Instance.from_location(
    driver="postgresql",
    location=host,
    options=options,
    id="Cleopatra",
)

print(inst)
assert inst.meta.uri == "http://onto-ns.com/meta/0.1/SimplePerson"
assert inst.dimensions == {}
assert inst.name == "Cleopatra"
assert inst.age == 2092
