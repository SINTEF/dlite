"""TEM example workflow - implemented as a OTEAPI pipeline."""
from otelib import OTEClient

from tripper import EMMO, MAP, Namespace

from paths import outdir, temdata


temimage = "6c8cm_008"
thumbnail = outdir / f"{temimage}.png"

# Namespaces
OTEIO = Namespace("http://emmo.info/oteio#")
TEMIMAGE = Namespace("http://onto-ns.com/meta/0.1/TEMImage#")
IMAGE = Namespace("http://onto-ns.com/meta/0.1/Image#")


client = OTEClient("python")

# Partial pipeline 1
tem_resource = client.create_dataresource(
    downloadUrl=f"{temdata}/{temimage}.dm3",
    mediaType="application/vnd.dlite-parse",
    configuration={
        "driver": "dm3",
        "label": f"{temimage}",
    },
)

tem_mapping = client.create_mapping(
    mappingType="mappings",
    prefixes={
        "temimage": str(TEMIMAGE),
        "map": str(MAP),
        "emmo": str(EMMO),
        "oteio": str(OTEIO),
    },
    triples=[
        (TEMIMAGE.filename,  MAP.mapsTo, OTEIO.FileName),
        (TEMIMAGE.data,      MAP.mapsTo, EMMO.Array),
        (TEMIMAGE.pixelUnit, MAP.mapsTo, EMMO.Unit),
        (TEMIMAGE.pixelSize, MAP.mapsTo, EMMO.Length),
        (TEMIMAGE.metadata,  MAP.mapsTo, OTEIO.Dictionary),
    ],
)

# Partial pipeline 2
image_mapping = client.create_mapping(
    mappingType="mappings",
    prefixes={
        "image": str(IMAGE),
        "map": str(MAP),
        "emmo": str(EMMO),
        "oteio": str(OTEIO),
    },
    triples=[
        (IMAGE.filename,  MAP.mapsTo, OTEIO.FileName),
        (IMAGE.data,      MAP.mapsTo, EMMO.Array),
    ],
)

image_generate = client.create_function(
    functionType="application/vnd.dlite-generate",
    configuration={
        "driver": "image",
        "location": str(thumbnail),
        "options": "equalize=true;as_gray=true;resize=256x256",
        "label": f"{temimage}",
    },
)



#forces_resource = client.create_dataresource(
#    downloadUrl=(inputdir / "forces.yaml").as_uri(),
#    mediaType="application/vnd.dlite-parse",
#    configuration={
#        "driver", "yaml",
#        "options": "mode=r",
#        "label": "forces",
#    },
#)
#
#convert = client.create_function(
#    functionType="application/vnd.dlite-convert",
#    configuration={
#        "module_name": "test_package.convert_module",
#        "function_name": "converter",
#        "inputs": [
#            {"label": "energy"},
#            {"label": "forces"},
#        ],
#        "outputs": [
#            {"label": "result"},
#        ],
#    },
#)





# Run pipeline
pipeline = (
    tem_resource >> tem_mapping >>
    image_mapping >> image_generate
)
pipeline.get()
