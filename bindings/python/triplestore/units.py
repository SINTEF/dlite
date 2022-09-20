"""Populates a pint unit registry from an ontology.

Further description here.

"""
from pint import UnitRegistry, Quantity
from triplestore import Triplestore

def load_qudt():
    print("Loading QUDT unit ontology.")
    ts = Triplestore(name="rdflib")
    ts.parse(source="http://qudt.org/2.1/vocab/unit")
    ts.parse(source="http://qudt.org/2.1/schema/qudt")
    print("Finished.")
    return ts



# Test code.

ts = load_qudt()

QUDTU = ts.bind("unit", "http://qudt.org/vocab/unit/", check=True)
QUDT = ts.bind("unit", "http://qudt.org/schema/qudt/", check=True)

for s, p, o in ts.triples([None, QUDT.hasDimensionVector, None]):
    print(s)

