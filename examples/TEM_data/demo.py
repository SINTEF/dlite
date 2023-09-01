"""This runs the
"""
from tripper import Triplestore

from paths import indir
from utils.utils import get_data, populate_triplestore


# Create a triplestore and populate it with triples
ts = Triplestore(backend="rdflib")
populate_triplestore(ts, indir / "resources.yaml")

# Namespace used for the data resources
PHYSMET = ts.bind("physmet", "https://www.ntnu.edu/physmet/data#")


# Get a thumbnail of physmet:TEM_6c8cm_008
get_data(ts, steps=(PHYSMET.TEM_6c8cm_008, PHYSMET.thumbnail_image))


# Get microscope setting for physmet:TEM_040
get_data(ts, steps=(PHYSMET.TEM_040, PHYSMET.microscope_settings))


# Get precipitation statistics for physmet:TEM_BF_100-at-m5-and-2_001
get_data(ts, steps=(
    PHYSMET["TEM_BF_100-at-m5-and-2_001"],
    PHYSMET.precipitate_statistics,
    PHYSMET.precip_stat_table,
))


# Get input to presipitation model based on the combination of the
# alloy composition and precipitation statistics obtained from
# postprocessing TEM image physmet:TEM_6c8cm_008.
get_data(ts, steps=(
    # Data sources
    PHYSMET.alloy_composition,
    PHYSMET.TEM_6c8cm_008,
    # Postprocessing
    PHYSMET.precipitate_statistics,
    # Data sink
    PHYSMET.precipitation_model_input,
))
