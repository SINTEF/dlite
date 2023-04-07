import sys
import re
import socket
from pathlib import Path

import dlite


# Paths
thisdir = Path(__file__).resolve().parent


def parse_pgconf():
    """Read storages/python/test-c/pgconf.h file and returns the tuple

        (host, user, database, password)

    Exit with code 44 (skip test) if the pgconf.h file does not exists.
    """
    rootdir = thisdir.parent.parent.parent
    pgconf = rootdir / 'storages/python/tests-c/pgconf.h'
    if not pgconf.exists():
        print(f"No configuration file: {pgconf}")
        print("For more info, see storages/python/README.md")
        sys.exit(44)
    regex = re.compile(r'^#define +(\w+) +"(\w+)"')
    with open(pgconf, 'rt') as f:
        d = {}
        for line in f:
            matchobj = regex.match(line)
            if matchobj:
                keyword, value = matchobj.groups()
                d[keyword.lower()] = value
    keywords = 'host,user,database,password'.split(',')
    return tuple(d.get(key) for key in keywords)


def ping_server(server="localhost", port=5432, timeout=3):
    """Exit with code 44 if the server cannot be contacted."""
    try:
        socket.setdefaulttimeout(timeout)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((server, port))
    except (OSError, ConnectionRefusedError) as exc:
        print(f"Cannot contact PostgreSQL server on {localhost}:{port}: {exc}")
        sys.exit(44)
    else:
        s.close()


# Check if postgresql server is running
ping_server()

# Add metadata to search path
dlite.storage_path.append(f'{thisdir}/Person.json')

# Load dataset
host, user, database, password = parse_pgconf()
inst = dlite.Instance.from_location(
    'json', f'{thisdir}/persons.json',
    id='51c0d700-9ab0-43ea-9183-6ea22012ebee',
)


# Save to postgresql DB
inst.save(
    f'postgresql://{host}?user={user};database={database};password={password}'
)
