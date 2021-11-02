#!/usr/bin/env python3
from pathlib import Path

import dlite


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
rootdir = thisdir.parent
workflow1dir = rootdir / '1-simple-workflow'
entitiesdir = rootdir / 'entities'
atomdata = workflow1dir / 'atomscaledata.json'
dlite.storage_path.append(f'{entitiesdir}/*.json')


mappings = [
    ('http://onto-ns.com/meta/0.1/Molecule#name', 'mo:mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Molecule#energy', 'mo:mapsTo',
     'chem:GroundStateEnergy'),
    ('http://onto-ns.com/meta/0.1/Substance#id', 'mo:mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Substance#molecule_energy', 'mo:mapsTo',
     'chem:GroundStateEnergy'),
]

Molecule = dlite.get_instance('http://onto-ns.com/meta/0.1/Molecule')
Substance = dlite.get_instance('http://onto-ns.com/meta/0.1/Substance')


def create_match(triples):
    """Returns a match functions for `triples`."""
    def match(s=None, p=None, o=None):
        """Returns generator over all triples that matches (s, p, o)."""
        return (t for t in triples if
                (s is None or t[0] == s) and
                (p is None or t[1] == p) and
                (o is None or t[2] == o))
    return match

match = create_match(mappings)

def match_first(s=None, p=None, o=None):
    """Returns the first match.  If there are no matches, ``(None, None, None)``
    is returned."""
    return next(iter(match(s, p, o) or ()), (None, None, None))


mapsTo = 'mo:mapsTo'

for prop1 in Molecule['properties']:
    uri1 = f'{Molecule.uri}#{prop1.name}'
    for _, _, o in match(uri1, mapsTo, None):
        for prop2 in Substance['properties']:
            uri2 = f'{Substance.uri}#{prop2.name}'
            for _, _, oo in match(uri2, mapsTo, None):
                print(uri1, o, uri2, oo)

    #print(prop1.name)
    #print(uri1)
    #print(list(match(uri1)))
    #print()

#for s, p, o in mappings:
