import sys
import re
import socket
from pathlib import Path

import dlite
from check_import import check_import


# Paths
thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"
outdir = thisdir / "output"
entitydir = thisdir / "entities"


def parse_pgconf():
    """Read storages/python/test-c/pgconf.h file and returns the tuple

        (host, user, database, password)

    Exit with code 44 (skip test) if the pgconf.h file does not exists.
    """
    rootdir = thisdir.parent.parent.parent
    pgconf = rootdir / "storages/python/tests-c/pgconf.h"
    if not pgconf.exists():
        print(f"No configuration file: {pgconf}")
        print("For more info, see storages/python/README.md")
        sys.exit(44)
    regex = re.compile(r'^#define +(\w+) +"(\w+)"')
    with open(pgconf, "rt") as f:
        d = {}
        for line in f:
            matchobj = regex.match(line)
            if matchobj:
                keyword, value = matchobj.groups()
                d[keyword.lower()] = value
    keywords = "host,user,database,password".split(",")
    return tuple(d.get(key) for key in keywords)


def ping_server(server="localhost", port=5432, timeout=3):
    """Exit with code 44 if the server cannot be contacted."""
    try:
        socket.setdefaulttimeout(timeout)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((server, port))
    except (OSError, ConnectionRefusedError) as exc:
        print(f"Cannot contact PostgreSQL server on {server}:{port}: {exc}")
        sys.exit(44)
    else:
        s.close()


# Check if postgresql server is running
check_import("psycopg", skip=True)
ping_server()

# Add metadata to search path
dlite.storage_path.append(f"{entitydir}/Person.json")
dlite.storage_path.append(f"{entitydir}/SimplePerson.json")

# Load dataset
host, user, database, password = parse_pgconf()
inst = dlite.Instance.from_location(
    "json",
    f"{indir}/persons.json",
    id="Cleopatra",
)


# Save to postgresql DB
inst.save(
    f"postgresql://{host}?user={user};database={database};password={password}"
)
