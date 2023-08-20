"""This test must be run after test_postgresql_write
"""
import sys
import re
from pathlib import Path

import dlite
from check_import import check_import
from test_postgresql1_write import parse_pgconf, ping_server


# Paths
thisdir = Path(__file__).resolve().parent


# Check if postgresql server is running
check_import("psycopg2", skip=True)
ping_server()

# Add metadata to search path
dlite.storage_path.append(f'{thisdir}/Person.json')


# Read from postgresql DB
host, user, database, password = parse_pgconf()
inst = dlite.Instance.from_location(
    driver='postgresql',
    location=host,
    options=f'user={user};database={database};password={password}',
    id='a1d8d35f-723c-5ea1-abaf-8fc8f1d0982f',
)

print(inst)
assert inst.uuid == 'a1d8d35f-723c-5ea1-abaf-8fc8f1d0982f'
assert inst.meta.uri == 'http://onto-ns.com/meta/0.1/Person'
assert inst.dimensions == {'N': 2}
assert inst.name == 'Jack Daniel'
assert inst.age == 42.0
assert inst.skills.tolist() == ['distilling', 'tasting']
