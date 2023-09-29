"""Demo: access and use TEM data"""
from tripper import RDF, Namespace, Triplestore

from tem_paths import indir, outdir
from utils.utils import get_data, populate_triplestore


# Create a triplestore and populate it with triples
ts = Triplestore(backend="rdflib")
populate_triplestore(ts, indir / "resources.yaml")
ts.serialize(outdir / "resources.ttl")

# Namespace used for the data resources
OTEIO = Namespace("http://emmo.info/oteio#")
PM = ts.bind("pm", "https://www.ntnu.edu/physmet/data#")


# List all data sources and sinks
print("Data sources:")
for source in ts.subjects(RDF.type, OTEIO.DataSource):
    print("  -", source)
print()
print("Data sinks:")
for sink in ts.subjects(RDF.type, OTEIO.DataSink):
    print("  -", sink)


# Get a thumbnail of pm:TEM_6c8cm_008
get_data(ts, steps=(PM.TEM_6c8cm_008, PM.thumbnail_image))


# Get microscope setting for pm:TEM_040
get_data(ts, steps=(PM.TEM_040, PM.microscope_settings))


# Get precipitation statistics for pm:TEM_BF_100-at-m5-and-2_001
get_data(ts, steps=(
    PM["TEM_BF_100-at-m5-and-2_001"],  # TEM image to analyse
    PM.image_analyser,                 # Postprocess with the image analyser
    PM.precipitate_statistics,         # How to present the result
))


# Get input to presipitation model based on the combination of the
# alloy composition and precipitation statistics obtained from
# postprocessing TEM image pm:TEM_6c8cm_008.
get_data(ts, steps=(
    PM.alloy_composition,          # Data source 1: alloy composition
    PM.TEM_6c8cm_008,              # Data source 2: TEM image
    PM.image_analyser,             # Postprocess with the image analyser
    PM.precipitation_model_input,  # How to present the result
))
