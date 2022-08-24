"""Populates a pint unit registry from an ontology.

Further description here.

"""
from pint import UnitRegistry, Quantity

def load_qudt():
    print("Loading QUDT unit ontology.")
    
    from triplestore import Triplestore
    ts = Triplestore(name="rdflib")
    
    ONTO = ts.bind("unit", "http://qudt.org/vocab/unit/", cachemode=2, check=True)
    
    print("Successful.")


load_qudt()