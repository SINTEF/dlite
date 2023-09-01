"""Prepare the triplestore.

Reads documentation of data resources from datadoc.yaml and serialise
it to turtle-formatted RDF.
"""

import yaml

from tripper import Triplestore
from tripper.convert import save_container, load_container

from paths import exampledir, indir, outdir

from utils.generate_pipeline import generate_partial_pipeline


ts = Triplestore(backend="rdflib")
RS = ts.bind("ex", "http://example.com/resources#")
PHYSMET = ts.bind("physmet": "https://www.ntnu.edu/physmet/data#")

with open(indir / "resources.yaml") as f:
    document = yaml.safe_load(f)

datadoc = document["data_resources"]
for name, config in datadoc.items():
    save_container(ts, config, name)

ts.serialize(outdir / "resources.ttl")
