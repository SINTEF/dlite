from pathlib import Path

import dlite
from dlite.dataset import metadata_to_rdf, add_dataset, get_dataset
from dlite.dataset import EMMO, EMMO_VERSIONIRI
from dlite.testutils import raises

try:
    from tripper import OWL, RDF, RDFS, XSD, Triplestore
except ModuleNotFoundError:
    import sys
    sys.exit(44)


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
chem = dlite.get_instance("http://onto-ns.com/meta/calm/0.1/Chemistry/aa6060")
fluid = dlite.get_instance("http://onto-ns.org/meta/dlite/0.1/FluidData")


base_iri = "http://emmo.info/domain/ex#"

#mappings = [

ts = Triplestore(backend="rdflib")
EX = ts.bind("", base_iri)
#add_dataset(ts, chem.meta, base_iri=base_iri)
add_dataset(ts, fluid, base_iri=base_iri)

# Add ontology
iri = base_iri.rstrip("/#")
ts.add_triples([
    (iri, RDF.type, OWL.Ontology),
    (iri, OWL.imports, EMMO_VERSIONIRI),
])


ts.serialize(outdir / "dataset.ttl")
