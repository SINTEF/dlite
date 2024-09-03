from pathlib import Path

import dlite
from dlite.testutils import raises, importskip
importskip("tripper")
importskip("rdflib")

from tripper import DCTERMS, MAP, OWL, RDF, RDFS, XSD, Triplestore
from tripper.utils import en

from dlite.dataset import add_dataset, add_data
from dlite.dataset import EMMO, EMMO_VERSIONIRI


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
    get_unit_iri("Atom")

with raises(MissingUnitError):
    # Ångström is not included in EMMO by default. It can be including by
    # importing https://w3id.org/emmo/1.0.0-rc1/disciplines/units/specialunits
    get_unit_iri("Å")


# To be fixed in issue https://github.com/SINTEF/dlite/issues/878
#from dlite.dataset import TS_EMMO
#TS_EMMO.parse("https://w3id.org/emmo/1.0.0-rc1/disciplines/units/prefixedunits", format="turtle")
#assert get_unit_iri("mm") == "https://w3id.org/emmo#MilliMetre"


# Test serialising Metadata as an EMMO dataset
# ============================================
Fluid = dlite.get_instance("http://onto-ns.org/meta/dlite/0.1/FluidData")

assert Fluid.get_hash() == (
    '9559cf53acd9f248d713e351ec432b515032f1fbaa345b03a104035c68d34f36'
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

# Make our ex: namespace an EMMO application ontology in the triplestore
iri = str(EX).rstrip("/#")
ts.add_triples([
    (iri, RDF.type, OWL.Ontology),
    (iri, DCTERMS.title, en("Test application ontology with a dataset.")),
    (iri, OWL.imports, EMMO_VERSIONIRI),
])

ts.serialize(outdir / "dataset.ttl")
