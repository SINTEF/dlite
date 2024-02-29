from pathlib import Path

import dlite
from dlite.dataset import metadata_to_rdf, add_dataset, get_dataset
from dlite.dataset import EMMO

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







dct = chem.meta.asdict()
triples = metadata_to_rdf(chem.meta)
meta = chem.meta

from dlite.dataset import get_unit_iri


unit = "V"
query = f"""
PREFIX  emmo: <{EMMO}>
PREFIX  rdf:  <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
PREFIX  rdfs: <http://www.w3.org/2000/01/rdf-schema#>
PREFIX  xsd:  <http://www.w3.org/2001/XMLSchema#>

SELECT ?unit
WHERE {{
  ?unit rdfs:subClassOf <{EMMO.UnitSymbol}>
}}
"""
#  ?unit <{EMMO.ucumCode}> ?symbol .
#  FILTER (?symbol="{unit}"^^xsd:string)

print(query)

t = EMMO._triplestore
r = t.query(query)
print(r)
