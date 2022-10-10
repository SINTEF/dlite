"""Populates a pint unit registry from an ontology.

Creates a Generator for the lines in the Pint unit registry.

"""
from email.policy import default
from xmlrpc.client import Boolean
from pint import UnitRegistry, Quantity
import re
from triplestore import Triplestore, RDFS
import warnings
from appdirs import user_cache_dir
import os

def load_qudt():
    ts = Triplestore(name="rdflib")
    ts.parse(source="http://qudt.org/2.1/vocab/unit")
    ts.parse(source="http://qudt.org/2.1/schema/qudt")
    return ts

def parse_qudt_dimension_vector(dimension_vector: str) -> dict:
    # Split the dimension vector string into separate dimensions.
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

    # Build the unit definition, dimension by dimension.
    result = ""
    for letter, unit in base_units.items():
        exponent = dimension_dict[letter]
        if int(dimension_dict[letter]) < 0:
            result += "/ " + unit + "**" + exponent[1:] + " "
        elif int(dimension_dict[letter]) > 0:
            result += "* " + unit + "**" + exponent + " "
    return result


def pint_registry_lines_from_qudt():
    ts = load_qudt()

    QUDTU = ts.bind("unit", "http://qudt.org/vocab/unit/", check=True)
    QUDT = ts.bind("unit", "http://qudt.org/schema/qudt/", check=True)
    DCTERMS = ts.bind("dcterms", "http://purl.org/dc/terms/")

    pint_registry_lines = []
    used_identifiers = []

    for s, p, o in ts.triples([None, QUDT.hasDimensionVector, None]):
        
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
            pint_definition_line = \
                f'{unit_name} = [{base_unit_dimensions[unit_name]}]'
        else:
            pint_definition_line = f'{unit_name} = {multiplier} {pint_definition}'

        if unit_name in used_identifiers:
            warnings.warn(f"OMITTING UNIT due to name conflict: {s}")
            continue
        else:
            used_identifiers.append(unit_name)
            used_identifiers_this_unit = [unit_name]

        # Add offset.
        if offset is not None:
            pint_definition_line += f'; offset: {offset}'

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

        pint_definition_line += f' = {symbol}'

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
                pint_definition_line += f' = {label}'
                used_identifiers.append(label)
                used_identifiers_this_unit.append(label)

        # Add IRI.
        pint_definition_line += f' = {s}'

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
                pint_definition_line += f' = {udunits_code}'

        pint_registry_lines.append(pint_definition_line)
    return pint_registry_lines


def prepare_cache_file_path(filename: str) -> str:
    cache_directory = user_cache_dir("dlite", "SINTEF")
    if not os.path.exists(cache_directory):
        os.mkdir(cache_directory)
    return os.path.join(cache_directory, filename)


def get_pint_registry(force_recreate = False) -> UnitRegistry:
    registry_file_path = prepare_cache_file_path("pint_unit_registry.txt")
    if force_recreate or not os.path.exists(registry_file_path):
        #pint_registry_lines = pint_registry_lines_from_qudt()
        pint_registry_lines = pint_registry_lines_from_qudt_experimental()
        with open(registry_file_path, "w") as f:
            for line in pint_registry_lines:
                f.write(f"{line}\n")
    
    return UnitRegistry(registry_file_path)


# Temporary experimental function that utilizes the PintIdentifiers class for handling
# the ambiguities.
def pint_registry_lines_from_qudt_experimental():
    ts = load_qudt()

    QUDTU = ts.bind("unit", "http://qudt.org/vocab/unit/", check=True)
    QUDT = ts.bind("unit", "http://qudt.org/schema/qudt/", check=True)
    DCTERMS = ts.bind("dcterms", "http://purl.org/dc/terms/")

    pint_registry_lines = []
    pint_definitions = {}
    identifiers = PintIdentifiers()

    # Explicit definition of which QUDT units that will serve as base units for
    # the pint unit registry. (i.e. the QUDT names for the SI units)
    base_unit_dimensions ={
        "M": "length",
        "SEC": "time",
        "A": "current",
        "CD": "luminosity",
        "KiloGM": "mass",
        "MOL": "substance",
        "K": "temperature", 
    }

    # Read info from all units.
    for s, p, o in ts.triples([None, QUDT.hasDimensionVector, None]):
        
        # Check if this unit has been replaced; then skip it.
        replaced_by = next(
            ts.objects(subject=s, predicate=DCTERMS.isReplacedBy), None)
        if replaced_by is not None:
            continue

        # Extract and parse the dimension vector.
        dimension_vector = o.split("/")[-1]
        pint_definition = pint_definition_string(parse_qudt_dimension_vector(
            dimension_vector))

        # Extract multiplier and offset.
        multiplier = next(
            ts.objects(subject=s, predicate=QUDT.conversionMultiplier), "1")
        offset = next(ts.objects(subject=s, predicate=QUDT.conversionOffset), None)

        pint_definitions[s] = {
            "unit_in_SI": pint_definition,
            "multiplier": multiplier,
            "offset": offset,
        }

        # Extract identifiers.
        unit = s.split("/")[-1]
        unit_name = unit.replace("-", "_")
        identifiers.add_identifier(URI=s, label_name="unit_name", prio=1, identifier=unit_name)
        # Can there be more than one symbol in QUDT?
        symbol = next(ts.objects(subject=s, predicate=QUDT.symbol), None)
        identifiers.add_identifier(URI=s, label_name="symbol", prio=2, identifier=symbol)
        for label in ts.objects(subject=s, predicate=RDFS.label):
            identifiers.add_identifier(URI=s, label_name="label", prio=3, identifier=label)
        udunits_code = next(
            ts.objects(subject=s, predicate=QUDT.udunitsCode), None)
        identifiers.add_identifier(URI=s, label_name="udunits_code", prio=4, identifier=udunits_code)

    identifiers.remove_ambiguities()

    # Build the pint unit registry lines.
    for URIb, definition in pint_definitions.items():

        unit_identifiers = identifiers.get_identifiers(URI=URIb)

        # Start constructing the pint definition line.
        unit_name = unit_identifiers["unit_name"]
        if unit_name in base_unit_dimensions.keys():
            pint_definition_line = \
                f'{unit_name} = [{base_unit_dimensions[unit_name]}]'
        else:
            pint_definition_line = f'{unit_name} = {definition["multiplier"]} {definition["unit_in_SI"]}'

        # if unit_name in used_identifiers:
        #     warnings.warn(f"OMITTING UNIT due to name conflict: {s}")
        #     continue
        # else:
        #     used_identifiers.append(unit_name)
        #     used_identifiers_this_unit = [unit_name]

        # Add offset.
        if definition["offset"] is not None:
            pint_definition_line += f'; offset: {definition["offset"]}'

        # Add symbol.
        symbol = unit_identifiers["symbol"]
        if symbol is None:
            symbol = "_"
        pint_definition_line += f' = {symbol}'

        # Add any labels.
        for label in unit_identifiers["labels"]:
            pint_definition_line += f' = {label}'

        # Add URI.
        pint_definition_line += f' = {URIb}'

        # Add udunits code.
        udunits_code = unit_identifiers["udunits_code"]
        if  udunits_code is not None:
            pint_definition_line += f' = {udunits_code}'

        pint_registry_lines.append(pint_definition_line)
    return pint_registry_lines


# Class for handling the various identifiers, with the functionality to remove
# any ambiguous definitions.
class PintIdentifiers:
    def __init__(self):
        self.URIs = []
        self.label_names = []
        self.prios = []
        self.identifiers = []

    def add_identifier(self, URI: str, label_name: str, prio: int, identifier:str):
        self.URIs.append(URI)
        self.label_names.append(label_name)
        self.prios.append(prio)
        self.identifiers.append(identifier)

    # Set ambiguous identifiers to None.
    # Keep the first occurence within each priority level.
    def remove_ambiguities(self):
        
        # Store used identifiers along with their URI.
        used_identifiers = {}

        # For each priority level, remove any ambiguities.
        for prio in sorted(list(set(self.prios))):
            inds_prio = [i for i,value in enumerate(self.prios) if value==prio]
            for i in inds_prio:
                if self.identifiers[i] is not None:
                    # Check if the identifier has already been used.
                    if self.identifiers[i] in used_identifiers.keys():
                        # Warn if this identifier belongs to another URI.
                        if self.URIs[i] is not used_identifiers[self.identifiers[i]]:
                            warnings.warn(f"Omitting {self.label_names[i]} \"{self.identifiers[i]}\" from {self.URIs[i]}")
                        self.identifiers[i] = None
                    else:
                        used_identifiers[self.identifiers[i]] = self.URIs[i]
    

    # Check if an identifier is valid for use as a particular label_name for a
    # particular unit.
    def is_valid_identifier(self, identifier:str, URI:str, label_name:str) -> Boolean:
        result = False
        identifier_index = self.identifiers.index(identifier)
        if (
            self.URIs[identifier_index] == URI and 
            self.label_names[identifier_index] == label_name
        ):
            result = True
        return result


    # Get a dict containing all identifiers for a given URI.
    def get_identifiers(self, URI:str) -> dict:
        identifiers = {}
        identifiers["labels"] = []

        inds = [i for i,value in enumerate(self.URIs) if value==URI]
        for i in inds:
            label_name = self.label_names[i]
            identifier = self.identifiers[i]
            if label_name == "unit_name":
                identifiers["unit_name"] = identifier
            elif label_name == "symbol":
                identifiers["symbol"] = identifier
            elif label_name == "labels":
                identifiers["labels"].append(identifier)
            elif label_name == "udunits_code":
                identifiers["udunits_code"] = identifier
        
        return identifiers