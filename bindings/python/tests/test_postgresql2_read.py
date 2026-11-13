"""This test must be run after test_postgresql_write
"""
from pathlib import Path

import dlite
from check_import import check_import
from dlite.testutils import serviceskip, skip_on_service_mismatch


# Paths
thisdir = Path(__file__).resolve().parent


# Check if postgresql server is running
check_import("psycopg", skip=True)
pg = serviceskip("postgresql")

# Add metadata to search path
dlite.storage_path.append(f"{thisdir}/entities/SimplePerson.json")


# Read from postgresql DB
with skip_on_service_mismatch("postgresql"):
    inst = dlite.Instance.from_location(
        driver="postgresql",
        location=f"{pg['host']}:{pg['port']}",
        options=(
            f"user={pg['user']};database={pg['database']};"
            f"password={pg['password']}"
        ),
        id="Cleopatra",
    )

print(inst)
assert inst.meta.uri == "http://onto-ns.com/meta/0.1/SimplePerson"
assert inst.dimensions == {}
assert inst.name == "Cleopatra"
assert inst.age == 2092
