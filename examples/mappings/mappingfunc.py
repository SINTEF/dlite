"""Mapping example using a collection."""
from pathlib import Path

import numpy as np

from tripper import EMMO, RDFS, Triplestore

import dlite


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
MOL = ts.bind("mol", "http://onto-ns.com/meta/0.2/Molecule#")

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
ts.map(AT.symbols, DON.ChemicalSymbol)
ts.map(RES.potential_energy, EMMO.PotentialEnergy)
ts.map(RES.forces, EMMO.Force)


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
ts.map(MOL.energy, EMMO.PotentialEnergy)
ts.map(MOL.maxforce, DON.MaxForce)
ts.map(MOL.formula, DON.Formula)





# 3. Add mapping functions -- ontologist
# --------------------------------------
from mappingfunc_module import formula, maximum, norm


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
    maximum,
    expects=[DON.ForceNorm],
    returns=[DON.MaxForce],
)


# 4. Instantiate a molecule -- modeller
# -------------------------------------
molecule, = coll.get_instances(metaid=MOL, property_mappings=True)

print("Molecule instance:")
print(molecule)


ts.serialize("output/mappingfunc.ttl")
