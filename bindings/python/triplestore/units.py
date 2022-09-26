"""Populates a pint unit registry from an ontology.

Creates a Generator for the lines in the Pint unit registry.

"""
from email.policy import default
from pint import UnitRegistry, Quantity
import re
from triplestore import Triplestore, RDFS

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
        if letter not in result.keys():
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

    result = ""
    for letter in base_units:
        exponent = dimension_dict[letter]
        if int(dimension_dict[letter]) < 0:
            result += "/ " + base_units[letter] + "**" + exponent[1:] + " "
        elif int(dimension_dict[letter]) > 0:
            result += "* " + base_units[letter] + "**" + exponent + " "
    return result

def pint_SI_base_units_definition() -> list:
    # SI units as defined in the pint default registry: pint/default_en.txt
    result = []
    result.append("meter = [length] = m = metre")
    result.append("second = [time] = s = sec")
    result.append("ampere = [current] = A = amp")
    result.append("candela = [luminosity] = cd = candle")
    result.append("gram = [mass] = g")
    result.append("mole = [substance] = mol")
    result.append("kelvin = [temperature]; offset: 0 = K = degK = °K = degree_Kelvin = degreeK")
    return result


# Test code.

ts = load_qudt()

QUDTU = ts.bind("unit", "http://qudt.org/vocab/unit/", check=True)
QUDT = ts.bind("unit", "http://qudt.org/schema/qudt/", check=True)

pint_registry_lines = pint_SI_base_units_definition()

for s, p, o in ts.triples([None, QUDT.hasDimensionVector, None]):
    print(s + " " + o)

    # Extract unit name.
    unit = s.split("/")[-1]
    unit_name = unit.replace("-", "_")

    # Extract and parse the dimension vector.
    dimension_vector = o.split("/")[-1]
    pint_definition = pint_definition_string(parse_qudt_dimension_vector(dimension_vector))

    # Extract remaining info.
    multiplier = next(ts.objects(subject=s, predicate=QUDT.conversionMultiplier), "1")
    offset = next(ts.objects(subject=s, predicate=QUDT.conversionOffset), None)
    # Can there be more than one symbol in QUDT?
    symbol = next(ts.objects(subject=s, predicate=QUDT.symbol), "_")
    labels = ts.objects(subject=s, predicate=RDFS.label)
    udunits_code = next(ts.objects(subject=s, predicate=QUDT.udunitsCode), None)

    # Start constructing the pint definition line.
    pint_definition_line = "".join([
        unit_name,
        " = ",
        multiplier,
        " ",
        pint_definition
        ])

    # Add offset.
    if offset is not None:
        pint_definition_line += "".join(["; offset: ", offset])

    # Add symbol.
    pint_definition_line += "".join([" = ", symbol])

    # Add any labels.
    for label in labels:
        pint_definition_line += "".join([" = ", label])

    # Add IRI.
    pint_definition_line += "".join([" = ", s])

    # Add udunits code.
    if udunits_code is not None:
        pint_definition_line += "".join([" = ", udunits_code])
    
    print(pint_definition_line)

    pint_registry_lines.append(pint_definition_line)

    # Syntax for pint unit definition with offset:
    # degC = degK; offset: 273.15 = celsius

    # General pint syntax:
    # millennium = 1e3 * year = _ = millennia (_ can be exchanged for a symbol)
    # Reference units and physical dimension:
    # second = [time] = s = sec
    # Prefixes:
    # yocto- = 10.0**-24 = y-

print("".join(["Number of registry lines = ", str(len(pint_registry_lines))]))

# Print pint registry definition to file.
with open("test_output.txt", "a") as f:
    for line in pint_registry_lines:
        f.write(f"{line}\n")
