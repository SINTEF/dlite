from pathlib import Path

import dlite
from dlite.testutils import serviceskip, skip_on_service_mismatch
from check_import import check_import


# Paths
thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"
outdir = thisdir / "output"
entitydir = thisdir / "entities"


# Skip this test if psycopg is not installed or the PostgreSQL server
# is not running.  The server and its credentials are configured in
# examples/docker-compose.yml (see dlite.testutils.service_config()).
check_import("psycopg", skip=True)
pg = serviceskip("postgresql")
location = f"{pg['host']}:{pg['port']}"
options = (
    f"user={pg['user']};database={pg['database']};password={pg['password']}"
)

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
with skip_on_service_mismatch("postgresql"):
    inst.save(f"postgresql://{location}?{options}")
