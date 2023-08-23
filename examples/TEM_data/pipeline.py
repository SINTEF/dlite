"""
"""
from pathlib import Path

import dlite

from tripper import EMMO, MAP, Namespace

import oteapi-dlite
from otelib import OTEClient


# Set up search paths
thisdir = Path(__file__).resolve().parent
plugindir = thisdir / "plugins"
entitydir = thisdir / "entities"
outdir = thisdir / "output"

dlite.python_storage_plugin_path.append(plugindir)
dlite.storage_path.append(entitydir)


# Create OTE client
client = OTEClient("python")

data_resource = client.create_dataresource(
    downloadUrl=f"https://folk.ntnu.no/friisj/temdata/6c8cm_008.dm3",
    mediaType="application/vnd.dlite-parse",
    configuration={
        "driver": "dm3",
        "label": "6c8cm_008",
    },
)

mapping_dm3 = client.create_mapping(
    mappingType="triples",
    prefixes={
        "emmo": f"{EMMO}",
        "map": f"{MAP}",
    },
    triples=[
    ],
)

mapping_image = client.create_mapping(
    mappingType="triples",
    prefixes={
        "emmo": f"{EMMO}",
        "map": f"{MAP}",
    },
    triples=[
    ],
)

generate = client.create_function(
    functionType="application/vnd.dlite-generate",
    configuration={
        "driver": "image",
        "location": f"{outdir}/6c8cm_008.png",
        "options": "equalize=true",
        "label": "6c8cm_008",
    },
)

pipeline = data_resource >> mapping_dm3 >> mapping_image >> function

pipeline.get()
