"""Populates a pint unit registry from an ontology.

Creates a Generator for the lines in the Pint unit registry.

"""
from email.policy import default
from pint import UnitRegistry, Quantity
import re
from triplestore import Triplestore, RDFS
import warnings

def load_qudt():
    ts = Triplestore(name="rdflib")
    ts.parse(source="http://qudt.org/2.1/vocab/unit")
    ts.parse(source="http://qudt.org/2.1/schema/qudt")
    return ts

def parse_qudt_dimension_vector(dimension_vector: str) -> dict:
    dimensions = re.findall(r'[AELIMHTD]-?[0-9]+', dimension_vector)
    
    result = {}
    for dimension in dimensions:
        result[dimension[0]] = dimension[1:]

    for letter in "AELIMHTD":
        if letter not in result.keys():
            raise Exception(f"Missing dimension \"{letter}\" in dimension vector \"{dimension_vector}\"")

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
    result.append("kilogram = 1000 * gram = kg")
    result.append(
        """@defaults
    group = international
    system = SI
@end
@system SI
    second
    meter
    kilogram
    ampere
    kelvin
    mole
    candela
@end""")
    return result


# Test code.

ts = load_qudt()

QUDTU = ts.bind("unit", "http://qudt.org/vocab/unit/", check=True)
QUDT = ts.bind("unit", "http://qudt.org/schema/qudt/", check=True)
DCTERMS = ts.bind("dcterms", "http://purl.org/dc/terms/")

#pint_registry_lines = pint_SI_base_units_definition()
pint_registry_lines = []
used_identifiers = []

for s, p, o in ts.triples([None, QUDT.hasDimensionVector, None]):
    #print(s + " " + o)

    # Check if this unit has been replaced; then skip it.
    replaced_by = next(
        ts.objects(subject=s, predicate=DCTERMS.isReplacedBy), None)
    if replaced_by is not None:
        continue

    # Extract unit name.
    unit = s.split("/")[-1]
    unit_name = unit.replace("-", "_")

    # Extract and parse the dimension vector.
    dimension_vector = o.split("/")[-1]
    pint_definition = pint_definition_string(parse_qudt_dimension_vector(
        dimension_vector))

    # Extract remaining info.
    multiplier = next(
        ts.objects(subject=s, predicate=QUDT.conversionMultiplier), "1")
    offset = next(ts.objects(subject=s, predicate=QUDT.conversionOffset), None)
    # Can there be more than one symbol in QUDT?
    symbol = next(ts.objects(subject=s, predicate=QUDT.symbol), "_")
    labels = ts.objects(subject=s, predicate=RDFS.label)
    udunits_code = next(
        ts.objects(subject=s, predicate=QUDT.udunitsCode), None)

    base_unit_dimensions ={
        "M": "length",
        "SEC": "time",
        "A": "current",
        "CD": "luminosity",
        "KiloGM": "mass",
        "MOL": "substance",
        "K": "temperature", 
    }

    # Start constructing the pint definition line.
    if unit_name in base_unit_dimensions.keys():
        pint_definition_line = "".join([
            unit_name,
            " = [",
            base_unit_dimensions[unit_name],
            "] ",
            ])
    else:
        pint_definition_line = "".join([
            unit_name,
            " = ",
            multiplier,
            " ",
            pint_definition
            ])

    if unit_name in used_identifiers:
        warnings.warn(f"OMITTING UNIT due to name conflict: {s}")
        continue
    else:
        used_identifiers.append(unit_name)
        used_identifiers_this_unit = [unit_name]

    # Add offset.
    if offset is not None:
        pint_definition_line += "".join(["; offset: ", offset])

    # Add symbol.
    if symbol != "_":
        if symbol in used_identifiers_this_unit:
            # This is OK, but we will not add the symbol since it duplicates
            # the name.
            symbol = "_"
        elif symbol in used_identifiers:
            # This is a conflict with another unit.
            warnings.warn(f"Omitting symbol \"{symbol}\" from {s}")
            symbol = "_"
        else:
            # No conflict; add the symbol to this unit.
            used_identifiers.append(symbol)
            used_identifiers_this_unit.append(symbol)

    pint_definition_line += "".join([" = ", symbol])

    # Add any labels.
    for label in labels:
        if label in used_identifiers_this_unit:
            # Conflict within the same unit; do nothing.
            pass
        elif label in used_identifiers:
            # Conflict with another unit.
            warnings.warn(f"Omitting label \"{label}\" from {s}")
        else:
            # No conflict.
            pint_definition_line += "".join([" = ", label])
            used_identifiers.append(label)
            used_identifiers_this_unit.append(label)

    # Add IRI.
    pint_definition_line += "".join([" = ", s])

    # Add udunits code.
    if udunits_code is not None:
        if udunits_code in used_identifiers_this_unit:
            # Conflict within the same unit; do nothing.
            pass
        elif udunits_code in used_identifiers:
            # Conflict with another unit.
            warnings.warn(f"Omitting UDUNITS code \"{udunits_code}\" from {s}")
        else:
            # No conflict.
            pint_definition_line += "".join([" = ", udunits_code])
    
    #print(pint_definition_line)

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
with open("test_output.txt", "w") as f:
    for line in pint_registry_lines:
        f.write(f"{line}\n")

# Populate an empty pint registry.
#ureg = UnitRegistry(None)
#for line in pint_registry_lines:
#    ureg.define(line)

# for i in range(0, 1751):
#     print("Line number: " + str(i))
#     print(pint_registry_lines[i])
#     with open("test_output.txt", "w") as f:
#         for line in pint_registry_lines[0:i]:
#             f.write(f"{line}\n")
#     ureg = UnitRegistry("test_output.txt")

ureg = UnitRegistry("test_output.txt")

# Test the registry.
test_quantity1 = 1234 * ureg.M
print(test_quantity1)

test_quantity2 = 2345.6 * ureg.W_PER_K
print("".join([str(test_quantity2), " = ", str(test_quantity2.to_base_units())]))

test_quantity3 = test_quantity1 * test_quantity2
print("".join([str(test_quantity3), " = ", str(test_quantity3.to_base_units()), " = ", "{:~}".format(test_quantity3.to_base_units())]))