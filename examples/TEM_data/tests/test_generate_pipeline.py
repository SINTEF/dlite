import yaml

from tripper import Triplestore
from tripper.convert import save_container, load_container

from otelib import OTEClient

from paths import exampledir, indir, outdir

from utils.generate_pipeline import generate_partial_pipeline, get_data


# Serialise the yaml resource documentation to turtle
ts = Triplestore(backend="rdflib")
RS = ts.bind("rs", "http://example.com/resources#")

with open(indir / "resources.yaml") as f:
    document = yaml.safe_load(f)

datadoc = document["data_documentation"]
for name, config in datadoc.items():
    save_container(ts, config, RS[name])

ts.serialize(outdir / "resources.ttl")


# Create a new triplestore initialised from resources
#ts = Triplestore(backend="rdflib")
#RS = ts.bind("rs", "http://example.com/resources#")
#ts.parse(indir / "resources.ttl")

#conf1 = load_container(ts, RS.tem_resource)


# Create OTE client
#client = OTEClient("python")
#pipe = generate_partial_pipeline(client, ts, RS.tem_resource)
#pipe.get()


get_data(ts, (RS.tem_resource, generate_image))
