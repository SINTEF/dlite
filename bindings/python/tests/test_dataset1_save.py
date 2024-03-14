from pathlib import Path

import dlite
from dlite.dataset import metadata_to_rdf, add_dataset, get_dataset
from dlite.dataset import EMMO
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
assert get_unit_iri("m/s") == "https://w3id.org/emmo#MetrePerSecond"

with raises(MissingUnitError):
    get_unit_iri("cm")






# Test serialising Metadata as an EMMO dataset
# ============================================
chem = dlite.get_instance("http://onto-ns.com/meta/calm/0.1/Chemistry/aa6060")
#chem = dlite.get_instance("http://onto-ns.com/meta/calm/0.1/Chemistry/c1eb2ab7-3fac-538b-b6f0-db2bf6530c92")


base_iri = "http://emmo.info/domain/ex#"

#mappings = [

ts = Triplestore(backend="rdflib")
EX = ts.bind("", base_iri)
add_dataset(ts, chem.meta, base_iri=base_iri)

# Add ontology
iri = base_iri.rstrip("/#")
ts.add_triples([
    (iri, RDF.type, OWL.Ontology),
    (iri, OWL.imports, EMMO._triplestore.backend.triplestore_url),
])


ts.serialize(outdir / "dataset.ttl")



# testing...
from tripper import Literal

t = EMMO._triplestore
