import yaml

from tripper import Triplestore
from temdata.generate_pipeline import generate_partial_pipeline, save_partial_pipeline, load_partial_pipeline

from paths import exampledir


with open(exampledir / "datadoc.yaml") as f:
    document = yaml.safe_load(f)

datadoc = document["data_documentation"]

conf1 = datadoc["tem_resource"]


ts = Triplestore(backend="rdflib")

EX = ts.bind("ex", "http://example.com/tem_example#")


save_partial_pipeline(ts, conf1, EX.tem_resource)

conf1b = load_partial_pipeline(ts, EX.tem_resource)


assert conf1b == conf1

print(ts.serialize())
