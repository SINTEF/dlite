"""Image analyser."""
import json

import dlite


def image_analyser(temimage):
    """Calculate precipitate statistics from a TEM image.

    Arguments:
        temimage: TEMImage instance to analyse.

    Returns:
        PrecipitateStatistics instance.
    ."""
    PS = dlite.get_instance("http://onto-ns.com/meta/0.1/PrecipitateStatistics")
    ps = PS(dimensions={"nconditions": 1})

    tags = json.loads(temimage.metadata)
    if tags.get("Microscope Info Illumination Mode") == "TEM":
        ps.alloy = ["Al-Mg-Si"]
        ps.condition = ["A"]
        ps.precipitate = ['beta"']
        ps.number_density = [30000]
        ps.avg_length = [50]
        ps.avg_crossection = [11]
        ps.volume_fraction = [2]
        ps.avg_atomic_volume = [0.019]
    else:
        ps.alloy = ["Al-Mg-Si"]
        ps.condition = ["B"]
        ps.precipitate = ['beta"']
        ps.number_density = [35000]
        ps.avg_length = [45]
        ps.avg_crossection = [8]
        ps.volume_fraction = [3]
        ps.avg_atomic_volume = [0.019]

    return ps
