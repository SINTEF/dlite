#!/usr/bin/env python
from pathlib import Path

import dlite
import dlite.mappings as dm

try:
    import pint
except ImportError as exc:
    import sys
    print(f"Skipped: {exc}")
    sys.exit(44)  # exit code marking the test to be skipped


# Configure search paths
thisdir = Path(__file__).parent.absolute()
dlite.storage_path.append(f'{thisdir}/*.json')


mappings = [
    ('http://onto-ns.com/meta/0.1/Molecule#name', ':mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Molecule#groundstate_energy', ':mapsTo',
     'chem:GroundStateEnergy'),
    ('http://onto-ns.com/meta/0.1/Substance#id', ':mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Substance#molecule_energy', ':mapsTo',
     'chem:GroundStateEnergy'),
]


match = dm.match_factory(mappings)
match_first = dm.match_factory(mappings, match_first=True)




# Check unitconvert_pint
assert dm.unitconvert_pint("km", 34, 'm') == 0.034
assert dm.unitconvert_pint("Å", 34, 'um') == 34e4
assert dm.unitconvert_pint("s", 1, 'hour') == 3600


routes = dm.mapping_route(
    target='http://onto-ns.com/meta/0.1/Substance#molecule_energy',
    sources=['http://onto-ns.com/meta/0.1/Molecule#groundstate_energy'],
    triples=mappings)
