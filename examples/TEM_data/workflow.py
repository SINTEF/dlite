"""TEM example workflow - implemented as a OTEAPI pipeline."""
from otelib import OTEClient

from tripper import EMMO, MAP, Namespace

from paths import outdir, temdata


temimage = "6c8cm_008"
thumbnail = outdir / f"{temimage}.png"

# Namespaces
MO = Namespace("http://emmo.info/microstructure#")
OTEIO = Namespace("http://emmo.info/oteio#")
TEMIMAGE = Namespace("http://onto-ns.com/meta/0.1/TEMImage#")
IMAGE = Namespace("http://onto-ns.com/meta/0.1/Image#")
PS = Namespace("http://onto-ns.com/meta/0.1/PrecipitateStatistics#")


client = OTEClient("python")


# Partial pipeline 1: Parse raw TEM image
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


# Partial pipeline 2: Generate image thumbnail
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


# Partial pipeline 3: Generate microscope settings
settings_generate = client.create_function(
    functionType="application/vnd.dlite-generate",
    configuration={
        "driver": "temsettings",
        "location": str(outdir / "temsettings.json"),
        "label": f"{temimage}",
    },
)


# Partial pipeline 4: Generate precipitate statistics
stat_convert = client.create_function(
    functionType="application/vnd.dlite-convert",
    configuration={
        "module_name": "temdata.image_analyser",
        "function_name": "image_analyser",
        "inputs": [
            {"label": f"{temimage}"},
        ],
        "outputs": [
            {"label": "precipitate_statistics"},
        ],
    },
)

stat_mapping = client.create_mapping(
    mappingType="mappings",
    prefixes={
        "ps": str(PS),
        "mo": str(MO),
        "map": str(MAP),
        "emmo": str(EMMO),
        "oteio": str(OTEIO),
    },
    triples=[
        (PS.alloy,           MAP.mapsTo, MO.alloy),
        (PS.condition,       MAP.mapsTo, EMMO.StateOfMatter),  # not really...
        (PS.precipitate,     MAP.mapsTo, MO.Precipitate),
        (PS.number_density,  MAP.mapsTo, EMMO.ParticleNumberDensity),
        (PS.avg_length,      MAP.mapsTo, EMMO.Length),
        (PS.avg_crossection, MAP.mapsTo, EMMO.Area),
        (PS.volume_fraction, MAP.mapsTo, EMMO.VolumeFraction),
    ],
)

stat_generate = client.create_function(
    functionType="application/vnd.dlite-generate",
    configuration={
        "driver": "csv",
        "location": str(outdir / "precipitate_statistics.csv"),
        "options": "mode=w",
        "label": "precipitate_statistics",
    },
)


# Partial pipeline 5: Parse alloy composition



# Partial pipeline 6: Precipitate model input



# Run pipeline
pipeline = (
    tem_resource >> tem_mapping >>
    image_mapping >> image_generate >>
    settings_generate >>
    stat_convert >> stat_mapping >> stat_generate
)
pipeline.get()
