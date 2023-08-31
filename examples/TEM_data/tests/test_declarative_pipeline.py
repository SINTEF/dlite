import yaml

from paths import exampledir
from temdata.generate_pipeline import generate_pipeline


with open(exampledir / "datadoc.yaml") as f:
    dct = yaml.safe_load(f)


datadoc = dct["data_documentation"]

pipelines = generate_pipeline(datadoc, "python")
