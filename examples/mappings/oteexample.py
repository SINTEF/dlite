"""Mapping example using OTELib."""
from pathlib import Path

import dlite
from dlite.testutils import importskip

importskip("tripper")
from tripper import EMMO, MAP, Triplestore

importskip("otelib")
from otelib import OTEClient

oteapi_dlite = importskip("oteapi_dlite", env_exitcode=None)



# Paths
thisdir = Path(__file__).absolute().parent
datadir = thisdir / "data"
entitydir = thisdir / "entities"
outdir = thisdir / "output"
dlite.storage_path.append(entitydir)


# Create temporary triplestore with mapping functions
# Here we use a collection backend, but we could equally well use rdflib
coll = dlite.Collection()
ts = Triplestore(backend="collection", collection=coll)

# Add namespaces
DON = ts.bind("don", "http://example.com/demo-ontology#")
AT = ts.bind("at", "http://onto-ns.com/meta/0.1/Structure#")
RES = ts.bind("res", "http://onto-ns.com/meta/0.1/CalcResult#")
MOL = ts.bind("mol", "http://onto-ns.com/meta/0.1/Molecule#")


# Create OTELib client
client = OTEClient("python")


# Documenting C3H6 structure
C3H6_resource = client.create_dataresource(
    downloadUrl=(datadir / "C3H6.json").as_uri(),
    mediaType="application/vnd.dlite-parse",
    configuration={
        "driver": "json",
        "label": "C3H6",
    },
)
C3H6_mapping = client.create_mapping(
    mappingType="mappings",
    prefixes={
        "at": str(AT),
        "map": str(MAP),
        "don": str(DON),
    },
    triples=[
        (AT.symbols,  MAP.mapsTo, DON.ChemicalSymbol),
    ],
)


# Documenting C3H6 calculation result
calc_resource = client.create_dataresource(
    downloadUrl=(datadir / "result.json").as_uri(),
    mediaType="application/vnd.dlite-parse",
    configuration={
        "driver": "json",
        "label": "result",
    },
)
calc_mapping = client.create_mapping(
    mappingType="mappings",
    prefixes={
        "res": str(RES),
        "map": str(MAP),
        "emmo": str(EMMO),
    },
    triples=[
        (RES.potential_energy,  MAP.mapsTo, EMMO.PotentialEnergy),
        (RES.forces,            MAP.mapsTo, EMMO.Force),
    ],
)


# Documenting Molecule
mol_mapping = client.create_mapping(
    mappingType="mappings",
    prefixes={
        "mol": str(MOL),
        "map": str(MAP),
        "emmo": str(EMMO),
        "don": str(DON),
    },
    triples=[
        (MOL.formula,  MAP.mapsTo, EMMO.ChemicalFormula),
        (MOL.energy,   MAP.mapsTo, EMMO.PotentialEnergy),
        (MOL.maxforce, MAP.mapsTo, DON.MaxForce),
    ],
)
mol_generate = client.create_function(
    functionType="application/vnd.dlite-generate",
    configuration={
        "driver": "json",
        "location": outdir / "molecule.json",
        "options": "mode=w",
        "datamodel": str(MOL),
    },
)


# Document mapping functions

# Import the mapping functions
from mappingfunc_module import formula, maximum, norm

# Use tripper to document the mapping functions
ts.add_function(
    formula,
    expects=[DON.ChemicalSymbol],
    returns=[EMMO.ChemicalFormula],
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

# Use mapping strategy since oteapi doesn't have anything better...
mapping_functions = client.create_mapping(
    mappingType="mappings",
    triples=list(ts.triples()),
)


# Execute pipeline
pipe = (
    mapping_functions >>
    C3H6_resource >> C3H6_mapping >>
    calc_resource >> calc_mapping >>
    mol_mapping >> mol_generate
)
pipe.get()


# Load and display generated molecule
molecule = dlite.Instance.from_location("json", outdir / "molecule.json")
print("Molecule instance (from OTE):")
print(molecule)
