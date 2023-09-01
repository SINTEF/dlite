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
#get_data(ts, steps=(PHYSMET.TEM_6c8cm_008, PHYSMET.thumbnail_image))


# Get microscope setting for physmet:TEM_040
#get_data(ts, steps=(PHYSMET.TEM_040, PHYSMET.microscope_settings))


# Get precipitation statistics for physmet:TEM_BF_100-at-m5-and-2_001
#get_data(ts, steps=(PHYSMET["TEM_BF_100-at-m5-and-2_001"], PHYSMET.precipitate_statistics))
#get_data(ts, steps=(PHYSMET.TEM_6c8cm_008, PHYSMET.precipitate_statistics, PHYSMET.precip_stat_table))


get_data(
    ts,
    steps=(
        # Input data sources
        PHYSMET.TEM_6c8cm_008,
        PHYSMET.alloy_composition,
        # Postprocessing
        PHYSMET.precipitate_statistics,
        # Data sink
        PHYSMET.precipitation_model_input,
    )
)
