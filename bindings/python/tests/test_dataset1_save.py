from pathlib import Path

import dlite
from dlite.dataset import metadata_to_rdf, add_dataset, get_dataset

from tripper import Triplestore


thisdir = Path(__file__).absolute().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"
dlite.storage_path.append(entitydir / "*.json")
dlite.storage_path.append(indir / "*.json")

chem = dlite.get_instance("http://onto-ns.com/meta/calm/0.1/Chemistry/aa6060")
#chem = dlite.get_instance("http://onto-ns.com/meta/calm/0.1/Chemistry/c1eb2ab7-3fac-538b-b6f0-db2bf6530c92")


ts = Triplestore(backend="rdflib")
add_dataset(ts, chem.meta, base_iri="http://emmo.info/domain/ex")

ts.serialize(outdir / "dataset.ttl")



dct = chem.meta.asdict()


triples = metadata_to_rdf(chem.meta)
