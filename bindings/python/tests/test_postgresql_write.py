import sys
import re
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
