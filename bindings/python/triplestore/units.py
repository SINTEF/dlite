"""Populates a pint unit registry from an ontology.
"""
import os
import re
import logging
from pint import UnitRegistry
from tripper import Triplestore, RDFS
from appdirs import user_cache_dir


def load_qudt():
    """Returns a Triplestore instance with QUDT pre-loaded."""
    ts = Triplestore(backend="rdflib")
    ts.parse(source="http://qudt.org/2.1/vocab/unit")
    ts.parse(source="http://qudt.org/2.1/schema/qudt")
    return ts


def parse_qudt_dimension_vector(dimension_vector: str) -> dict:
    """Split the dimension vector string into separate dimensions."""
    dimensions = re.findall(r'[AELIMHTD]-?[0-9]+', dimension_vector)

    result = {}
    for dimension in dimensions:
        result[dimension[0]] = dimension[1:]

    for letter in "AELIMHTD":
        if letter not in result.keys():
            raise Exception(
                f'Missing dimension "{letter}" in dimension vector '
                f'"{dimension_vector}"'
            )
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
    result = []
    for letter, unit in base_units.items():
        exponent = dimension_dict[letter]
        if int(dimension_dict[letter]) < 0:
            result.append(f"/ {unit}**{exponent[1:]} ")
        elif int(dimension_dict[letter]) > 0:
            result.append(f"* {unit}**{exponent} ")
    return "".join(result)


def prepare_cache_file_path(filename: str) -> str:
    """Return cache file name."""
    cache_directory = user_cache_dir("dlite")
    if not os.path.exists(cache_directory):
        os.makedirs(cache_directory)
    return os.path.join(cache_directory, filename)


def get_pint_registry(sources=('qudt', ), force_recreate=False) -> UnitRegistry:
    """Load units from one or more unit sources into a Pint unit registry.

    Arguments:
        sources: Sequence of unit sources to load.  The sources are loaded
            in the provided order. In case of conflicts, the source listed
            first has precedence.
        force_recreate: Whether to recreate the unit registry cache.

    Returns:
        Pint unit registry.
    """
    registry_file_path = prepare_cache_file_path("pint_unit_registry.txt")
    if force_recreate or not os.path.exists(registry_file_path):
        with open(registry_file_path, "w", encoding="utf8") as f:
            f.write("\n".join(pint_prefix_lines()) + "\n")
        for source in sources:
            pint_registry_lines = pint_registry_lines_from_qudt()
            with open(registry_file_path, "a", encoding="utf8") as f:
                f.write("\n".join(pint_registry_lines) + "\n")

    ureg = UnitRegistry(registry_file_path)
    #ureg.default_format = "~P" #symbols, pretty print
    ureg.default_format = "~" #symbols, standard print (preferred)
    #ureg.default_format = "~C" #symbols, compact print
    return ureg


def pint_prefix_lines():
    # Decimal prefixes from pint's default_en.txt registry.
    prefixes = ['quecto- = 1e-30 = q-',
        'ronto- = 1e-27 = r-',
        'yocto- = 1e-24 = y-',
        'zepto- = 1e-21 = z-',
        'atto- =  1e-18 = a-',
        'femto- = 1e-15 = f-',
        'pico- =  1e-12 = p-',
        'nano- =  1e-9  = n-',
        'micro- = 1e-6  = µ- = μ- = u-',
        'milli- = 1e-3  = m-',
        'centi- = 1e-2  = c-',
        'deci- =  1e-1  = d-',
        'deca- =  1e+1  = da- = deka-',
        'hecto- = 1e2   = h-',
        'kilo- =  1e3   = k-',
        'mega- =  1e6   = M-',
        'giga- =  1e9   = G-',
        'tera- =  1e12  = T-',
        'peta- =  1e15  = P-',
        'exa- =   1e18  = E-',
        'zetta- = 1e21  = Z-',
        'yotta- = 1e24  = Y-',
        'ronna- = 1e27 = R-',
        'quetta- = 1e30 = Q-',
    ]
    return prefixes


def prefix_names():
    lines = pint_prefix_lines()
    for i in range(len(lines)):
        lines[i] = lines[i].split(" ")[0].replace("-", "")
    return lines


def pint_registry_lines_from_qudt():
    ts = load_qudt()

    QUDTU = ts.bind("unit", "http://qudt.org/vocab/unit/", check=True)
    QUDT = ts.bind("unit", "http://qudt.org/schema/qudt/", check=True)
    DCTERMS = ts.bind("dcterms", "http://purl.org/dc/terms/")

    pint_registry_lines = []
    pint_definitions = {}
    identifiers = PintIdentifiers()

    # Explicit definition of which QUDT units that will serve as base units for
    # the pint unit registry. (i.e. the QUDT names for the SI units and the
    # name of their physical dimension)
    #base_unit_dimensions ={
    #    "M": "length",
    #    "SEC": "time",
    #    "A": "current",
    #    "CD": "luminosity",
    #    "KiloGM": "mass",
    #   "MOL": "substance",
    #    "K": "temperature",
    #}

    # Base units defined by rdfs:label (instead of QUDT name):
    base_unit_dimensions ={
        "Meter": "length",
        "Second": "time",
        "Ampere": "current",
        "Candela": "luminosity",
        "Kilogram": "mass",
        "Mole": "substance",
        "Kelvin": "temperature",
    }    

    # Read info from all units.
    for s, p, o in ts.triples([None, QUDT.hasDimensionVector, None]):

        # Check if this unit has been replaced; then skip it.
        replaced_by = next(
            ts.objects(subject=s, predicate=DCTERMS.isReplacedBy), None)
        if replaced_by is not None:
            continue

        unit = s.split("/")[-1]
        unit_name = unit.replace("-", "_")        

        omit_prefixed_unit = False
        for prefix in prefix_names():
            if unit_name.startswith('KiloGM'):
                pass
            elif unit_name.lower().startswith(prefix):
                omit_prefixed_unit = True
                break

        if omit_prefixed_unit:
            continue

        # Extract and parse the dimension vector.
        dimension_vector = o.split("/")[-1]
        pint_definition = pint_definition_string(parse_qudt_dimension_vector(
            dimension_vector))

        # Extract multiplier and offset.
        multiplier = next(
            ts.objects(subject=s, predicate=QUDT.conversionMultiplier), "1")
        offset = next(ts.objects(subject=s, predicate=QUDT.conversionOffset),
                      None)

        pint_definitions[s] = {
            "unit_in_SI": pint_definition,
            "multiplier": multiplier,
            "offset": offset,
        }

        # Extract identifiers.
        pint_name_is_set = False
        prio_downgrade = 2
        for label in ts.objects(subject=s, predicate=RDFS.label):
            label = label.replace(" ", "_")
            label = label.replace("-", "_")
            if pint_name_is_set:
                identifiers.add_identifier(
                    URI=s, label_name="label", prio=5, identifier=label)
            else:
                if label in base_unit_dimensions.keys():
                    prio_downgrade = 0
                identifiers.add_identifier(
                    URI=s, label_name="unit_name", prio=1+prio_downgrade, identifier=label)
                pint_name_is_set = True

        identifiers.add_identifier(
            URI=s, label_name="label", prio=6, identifier=unit_name)
        # Can there be more than one symbol in QUDT?
        symbol = next(ts.objects(subject=s, predicate=QUDT.symbol), None)
        if symbol is not None:
            symbol = symbol.replace(" ", "_")
        identifiers.add_identifier(
            URI=s, label_name="symbol", prio=2+prio_downgrade, identifier=symbol)
        udunits_code = next(
            ts.objects(subject=s, predicate=QUDT.udunitsCode), None)
        if udunits_code is not None:
            udunits_code = udunits_code.replace(" ", "_")
        identifiers.add_identifier(
            URI=s, label_name="udunits_code", prio=7, identifier=udunits_code)

    identifiers.remove_ambiguities()

    # Build the pint unit registry lines.
    for URIb, definition in pint_definitions.items():

        unit_identifiers = identifiers.get_identifiers(URI=URIb)

        # Start constructing the pint definition line.
        unit_name = unit_identifiers["unit_name"]
        if unit_name is None:
            logging.warning(f'Omitting UNIT {URIb} due to name conflict.')
            continue
        if unit_name in base_unit_dimensions.keys():
            pint_definition_line = (
                f'{unit_name} = [{base_unit_dimensions[unit_name]}]'
            )
        else:
            pint_definition_line = (
                f'{unit_name} = {definition["multiplier"]} '
                f'{definition["unit_in_SI"]}'
            )

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
            if label is not None:
                pint_definition_line += f' = {label}'

        # Add URI.
        pint_definition_line += f' = {URIb}'

        # Add udunits code.
        udunits_code = unit_identifiers["udunits_code"]
        if  udunits_code is not None:
            pint_definition_line += f' = {udunits_code}'

        pint_registry_lines.append(pint_definition_line)
    return pint_registry_lines


class PintIdentifiers:
    """Class for handling the various identifiers, with the functionality
    to remove any ambiguous definitions.
    """
    def __init__(self):
        self.URIs = []
        self.label_names = []
        self.prios = []
        self.identifiers = []

    def add_identifier(self, URI: str, label_name: str, prio: int,
                       identifier:str):
        self.URIs.append(URI)
        self.label_names.append(label_name)
        self.prios.append(prio)
        self.identifiers.append(identifier)

    def remove_ambiguities(self):
        """Remove ambiguities.

        Set ambiguous identifiers to None.
        Keep the first occurence within each priority level.
        """
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
                        URI_of_identifier = used_identifiers[self.identifiers[i]]
                        if self.URIs[i] is not URI_of_identifier:
                            logging.warning(
                                f'Omitting {self.label_names[i]} '
                                f'"{self.identifiers[i]}" from {self.URIs[i]} '
                                f'(the identifier is used for '
                                f'{URI_of_identifier})'
                            )
                        self.identifiers[i] = None
                    else:
                        used_identifiers[self.identifiers[i]] = self.URIs[i]


    def is_valid_identifier(self, identifier: str, URI: str,
                            label_name: str) -> bool:
        """Check if an identifier is valid for use as a particular
        label_name for a particular unit.
        """
        identifier_index = self.identifiers.index(identifier)
        return (self.URIs[identifier_index] == URI and
                self.label_names[identifier_index] == label_name)

    def get_identifiers(self, URI:str) -> dict:
        """Returns a dict containing all identifiers for a given URI."""
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
            elif label_name == "label":
                identifiers["labels"].append(identifier)
            elif label_name == "udunits_code":
                identifiers["udunits_code"] = identifier

        return identifiers
