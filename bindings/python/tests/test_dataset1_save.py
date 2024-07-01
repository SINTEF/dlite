from pathlib import Path

try:
    from tripper import DCTERMS, MAP, OWL, RDF, RDFS, XSD, Triplestore
    from tripper.utils import en
except ModuleNotFoundError:
    import sys
    sys.exit(44)

import dlite
from dlite.dataset import add_dataset, add_data
from dlite.dataset import EMMO, EMMO_VERSIONIRI
from dlite.testutils import raises


thisdir = Path(__file__).absolute().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"
dlite.storage_path.append(entitydir / "*.json")
dlite.storage_path.append(indir / "*.json")


# Test help functions
# ===================
from dlite.dataset import MissingUnitError, get_unit_iri

assert get_unit_iri("Kelvin") == "https://w3id.org/emmo#Kelvin"
assert get_unit_iri("K") == "https://w3id.org/emmo#Kelvin"
assert get_unit_iri("°C") == "https://w3id.org/emmo#DegreeCelsius"
assert get_unit_iri("m/s") == "https://w3id.org/emmo#MetrePerSecond"

with raises(MissingUnitError):
    get_unit_iri("cm")



# Test serialising Metadata as an EMMO dataset
# ============================================
Fluid = dlite.get_instance("http://onto-ns.org/meta/dlite/0.1/FluidData")

assert Fluid.get_hash() == (
    '4739a3820ced457d07447c8916112021a0fbda9cbc97758e40b67369e34c00b4'
)

ts = Triplestore(backend="rdflib")
EX = ts.bind("ex", "https://w3id.org/emmo/application/ex/0.2/")
FLUID = ts.bind("fluid", "http://onto-ns.org/meta/dlite/0.1/FluidData#")

mappings = [
    (FLUID,                  EMMO.isDescriptionFor, EMMO.Fluid),
    (FLUID.LJPotential,      MAP.mapsTo, EMMO.String),
    (FLUID.LJPotential,      EMMO.isDescriptionFor, EMMO.MolecularEntity),
    (FLUID.TemperatureField, MAP.mapsTo, EMMO.ThermodynamicTemperature),
    (FLUID.ntimes,           MAP.mapsTo, EMMO.Time),
    (FLUID.npositions,       MAP.mapsTo, EMMO.Position),
]
#add_dataset(ts, chem.meta, base_iri=base_iri)
add_dataset(ts, Fluid, iri=EX.FluidData, mappings=mappings)


# Test serialising data instances to KB
# =====================================

# Create instances
fluid1 = Fluid(dimensions={"ntimes":2, "npositions": 3}, id="fluid1")
fluid1.LJPotential = "WaterPot"
fluid1.TemperatureField = [[20., 24., 28.], [22, 26, 29]]

uuid2 = dlite.get_uuid("fluid2")  # just to ensure persistent uuid...
fluid2 = Fluid(dimensions={"ntimes":2, "npositions": 4}, id=uuid2)
fluid2.LJPotential = "AcetonePot"
fluid2.TemperatureField = [[20., 24., 28., 32.], [22, 26, 30, 34]]

assert fluid1.get_hash() == (
    "412b7387f8c13c9d1aaa65ca21d59957be5635b41c7c3851b268de508817f7f8"
)
assert fluid2.get_hash() == (
    "c4289ff03f880526fc0f87038302673e44101c2b648be2c57a4db84fe6779f67"
)

add_data(ts, fluid1)
add_data(ts, fluid2)



# Add ontology and save to file
# =============================

# Add ontology
iri = str(EX).rstrip("/#")
ts.add_triples([
    (iri, RDF.type, OWL.Ontology),
    (iri, DCTERMS.title, en("Test application ontology with a dataset.")),
    (iri, OWL.imports, EMMO_VERSIONIRI),
])

ts.serialize(outdir / "dataset.ttl")
