"""Populates a pint unit registry from an ontology.

Further description here.

"""
from pint import UnitRegistry, Quantity
import re
from triplestore import Triplestore

def load_qudt():
    print("Loading QUDT unit ontology.")
    ts = Triplestore(name="rdflib")
    ts.parse(source="http://qudt.org/2.1/vocab/unit")
    ts.parse(source="http://qudt.org/2.1/schema/qudt")
    print("Finished.")
    return ts

def parse_qudt_dimension_vector(dimension_vector: str) -> dict:
    dimensions = re.findall(r'[AELIMHTD]-?[0-9]+', dimension_vector)
    result = {}
    for dimension in dimensions:
        result[dimension[0]] = dimension[1:]

    expected_keys = ["A", "E", "L", "I", "M", "H", "T", "D"]
    for letter in expected_keys:
        if not result.has_key(letter):
            raise Exception("Missing dimension \"" + letter + "\" in dimension vector " + dimension_vector)

    return result


def pint_definition_string(dimension_dict: dict) -> str:
    # Base units defined by:
    # https://qudt.org/schema/qudt/QuantityKindDimensionVector
    # https://qudt.org/vocab/sou/SI
    base_units = {
        "A": "mol",
        "E": "A",
        "L": "m",
        "I": "cd",
        "M": "kg",
        "H": "K",
        "T": "s",
        "D": "1",
    }

# Test code.

ts = load_qudt()

QUDTU = ts.bind("unit", "http://qudt.org/vocab/unit/", check=True)
QUDT = ts.bind("unit", "http://qudt.org/schema/qudt/", check=True)

for s, p, o in ts.triples([None, QUDT.hasDimensionVector, None]):
    print(s + " " + o)

    unit = s.split("/")[-1]
    dimension_vector = o.split("/")[-1]



