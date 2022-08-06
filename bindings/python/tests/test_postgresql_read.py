"""This test must be run after test_postgresql_write
"""
import sys
import re
from pathlib import Path

import dlite

from test_postgresql_write import parse_pgconf


# Paths
thisdir = Path(__file__).resolve().parent


# Add metadata to search path
dlite.storage_path.append(f'{thisdir}/Person.json')


# Read from postgresql DB
host, user, database, password = parse_pgconf()
inst = dlite.Instance.from_location(
    driver='postgresql',
    location=host,
    options=f'user={user};database={database};password={password}',
    id='51c0d700-9ab0-43ea-9183-6ea22012ebee',
)

print(inst)
