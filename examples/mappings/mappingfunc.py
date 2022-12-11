"""Mapping example using a collection."""
from pathlib import Path

import numpy as np

from tripper import DM, EMMO, RDFS, Triplestore

import dlite
from dlite.mappings import instantiate


# Paths
thisdir = Path(__file__).absolute().parent
datadir = thisdir / "data"
entitydir = thisdir / "entities"
dlite.storage_path.append(f"{entitydir}/*.json")

# Create collection -- our knowledge base in this example
coll = dlite.Collection()

# Create a triplestore "view" of our knowledge base
ts = Triplestore(backend="collection", collection=coll)

# Add namespaces
DON = ts.bind("don", "http://example.com/demo-ontology#")
AT = ts.bind("at", "http://onto-ns.com/meta/0.1/Structure#")
RES = ts.bind("res", "http://onto-ns.com/meta/0.1/CalcResult#")
MOL = ts.bind("mol", "http://onto-ns.com/meta/0.1/Molecule#")

# Load data
C3H6 = dlite.Instance.from_location("json", datadir / "C3H6.json")
result = dlite.Instance.from_location("json", datadir / "result.json")
coll.add("C3H6", C3H6)
coll.add("result", result)


# 1. Map input datamodels -- data provider + ontologist
# -----------------------------------------------------

# Ontologist adds missing concepts
# Note: This step is conceptually important, but is actually not needed for
# the demo to work...
# TODO: add simple interface for this, include elucidation, relations+++
ts.add_triples(
    [
        (DON.ChemicalSymbol, RDFS.subClassOf, EMMO.ChemicalElement),
    ]
)

# Add mappings for the input data models -- data provider
ts.add_mapsTo(DON.ChemicalSymbol, AT.symbols)
ts.add_mapsTo(EMMO.PotentialEnergy, RES.potential_energy)
ts.add_mapsTo(EMMO.Force, RES.forces)


# 2. Map output datamodels -- modeller + ontologist
# -------------------------------------------------

# Ontologist adds missing concepts
ts.add_triples(
    [
        (DON.ForceNorm, RDFS.subClassOf, EMMO.Force),
        (DON.MaxForce, RDFS.subClassOf, EMMO.Force),
    ]
)

# Add mappings for the output data model -- modeller
ts.add_mapsTo(EMMO.PotentialEnergy, MOL.energy)
ts.add_mapsTo(DON.MaxForce, MOL.maxforce)
ts.add_mapsTo(DON.Formula, MOL.formula)


# 3. Add mapping functions -- ontologist
# --------------------------------------

# TODO: make these globally available as an installable package
def formula(symbols):
    """Convert a list of atomic symbols to a chemical formula."""
    lst = symbols.tolist()
    return "".join(f"{c}{lst.count(c)}" for c in set(lst))


def norm(array, axis=-1):
    """Returns the norm array along the given axis (default the last)."""
    # Note that `array` is a Quantity object.  The returned value
    # will also be a Quantity object with the same unit.  Hence, the
    # unit is always handled explicitly.  This makes it possible for
    # conversion function to change unit as well.
    return np.sqrt(np.sum(array**2, axis=axis))


def max(vector):
    """Returns the largest element."""
    return np.max(vector)


# Add mappings for conversion functions -- ontologist
ts.add_function(
    formula,
    expects=[DON.ChemicalSymbol],
    returns=[DON.Formula],
)
ts.add_function(
    norm,
    expects=[EMMO.Force],
    returns=[DON.ForceNorm],
)
ts.add_function(
    max,
    expects=[DON.ForceNorm],
    returns=[DON.MaxForce],
)


# 4. Instantiate a molecule -- modeller
# -------------------------------------
molecule, = coll.get_instances(
    metaid=MOL, property_mappings=True, function_repo=ts.function_repo,
)

print("Molecule instance:")
print(molecule)
