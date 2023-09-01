"""This runs the
"""
import yaml

from tripper import Triplestore
from tripper.convert import save_container, load_container

from paths import indir, outdir

from utils.generate_pipeline import get_data, populate_triplestore


ts = Triplestore(backend="rdflib")
RS = ts.bind("ex", "http://example.com/resources#")
PHYSMET = ts.bind("physmet", "https://www.ntnu.edu/physmet/data#")


populate_triplestore(ts, indir / "resources.yaml")
ts.serialize(outdir / "resources.ttl")

# Get a thumbnail of TEM image 6c8cm_008
get_data(ts, steps=(PHYSMET.TEM_6c8cm_008, PHYSMET.generate_image))
