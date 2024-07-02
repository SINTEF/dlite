from pathlib import Path

try:
    from tripper import MAP, Triplestore
except ModuleNotFoundError:
    import sys
    sys.exit(44)

import dlite
from dlite.dataset import EMMO, get_dataset, get_data
from dlite.testutils import raises


thisdir = Path(__file__).absolute().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"


# Test load Metadata from triplestore
# ===================================

ts = Triplestore(backend="rdflib")
ts.parse(outdir / "dataset.ttl")
EX = ts.namespaces["ex"]
FLUID = ts.bind("fluid", "http://onto-ns.org/meta/dlite/0.1/FluidData#")

Fluid, mappings = get_dataset(ts, iri=EX.FluidData)

print(Fluid)

# Check that the loaded datamodel looks as expected
assert Fluid.uri == str(FLUID).rstrip("#")
assert Fluid.dimnames() == ["ntimes", "npositions"]
assert len(Fluid.props) == 2
assert Fluid.props["TemperatureField"].unit == "°C"

# Check that we get the exact same hash as in test_dataset1_save.py
assert Fluid.get_hash() == (
    '4739a3820ced457d07447c8916112021a0fbda9cbc97758e40b67369e34c00b4'
)

# Check that we get the exact same mappings as provided
assert set(mappings) == {
    (Fluid.uri,              EMMO.isDescriptionFor, EMMO.Fluid),
    (FLUID.LJPotential,      MAP.mapsTo,            EMMO.String),
    (FLUID.LJPotential,      EMMO.isDescriptionFor, EMMO.MolecularEntity),
    (FLUID.TemperatureField, MAP.mapsTo, EMMO.ThermodynamicTemperature),
    (FLUID.ntimes,           MAP.mapsTo, EMMO.Time),
    (FLUID.npositions,       MAP.mapsTo, EMMO.Position),
}


# Test load data instances from triplestore
# =========================================

uuid2 = dlite.get_uuid("fluid2")  # persistent uuid...
fluid1, mappings1 = get_data(ts, iri=f"{EX.FluidData}/fluid1")
fluid2, mappings2 = get_data(ts, iri=f"{EX.FluidData}/{uuid2}")

print("---------")
print(fluid1)

assert fluid1.meta == Fluid
assert fluid1.uri == "fluid1"
assert fluid2.meta == Fluid
assert fluid2.uri == None

# Check that the instances have the exact same hash values as
# when they were created
assert fluid1.get_hash() == (
    "412b7387f8c13c9d1aaa65ca21d59957be5635b41c7c3851b268de508817f7f8"
)
assert fluid2.get_hash() == (
    "c4289ff03f880526fc0f87038302673e44101c2b648be2c57a4db84fe6779f67"
)
