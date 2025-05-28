"""TEM example pipeline."""
from paths import indir, outdir, temdata

from otelib import OTEClient

from tripper import EMMO, MAP, OTEIO, Namespace


temimage = "6c8cm_008"
thumbnail = outdir / f"{temimage}.png"

# Namespaces
MO = Namespace("https://w3id.org/emmo/domain/microstructure#")
TEMIMAGE = Namespace("http://onto-ns.com/meta/characterisation/0.1/TEMImage#")
IMAGE = Namespace("http://onto-ns.com/meta/0.1/Image#")
PS = Namespace("http://onto-ns.com/meta/0.1/PrecipitateStatistics#")
COMP = Namespace("http://onto-ns.com/meta/0.1/Composition#")
CHEM = Namespace("http://onto-ns.com/meta/0.3/Chemistry#")

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
        (PS.alloy,             MAP.mapsTo, MO.Alloy),
        (PS.condition,         MAP.mapsTo, MO.AlloyCondition),
        (PS.precipitate,       MAP.mapsTo, MO.Precipitate),
        (PS.number_density,    MAP.mapsTo, EMMO.ParticleNumberDensity),
        (PS.avg_length,        MAP.mapsTo, MO.PrecipitateLength),
        (PS.avg_crossection,   MAP.mapsTo, MO.PrecipitateCrossection),
        (PS.volume_fraction,   MAP.mapsTo, EMMO.VolumeFraction),
        (PS.avg_atomic_volume, MAP.mapsTo, MO.AtomicVolume),
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
comp_resource = client.create_dataresource(
    downloadUrl=(indir / "composition.csv").as_uri(),
    mediaType="application/vnd.dlite-parse",
    configuration={
        "driver": "composition",
        "label": "composition",
    },
)

comp_mapping = client.create_mapping(
    mappingType="mappings",
    prefixes={
        "comp": str(COMP),
        "mo": str(MO),
        "map": str(MAP),
        "emmo": str(EMMO),
    },
    triples=[
        (COMP.alloy,               MAP.mapsTo, MO.Alloy),
        (COMP.elements,            MAP.mapsTo, EMMO.ChemicalElement),
        (COMP.phases,              MAP.mapsTo, MO.Phase),
        (COMP.nominal_composition, MAP.mapsTo, MO.NominalComposition),
        (COMP.phase_compositions,  MAP.mapsTo, MO.PhaseComposition),
    ],
)


# Partial pipeline 6: Precipitate model input
chem_mapping = client.create_mapping(
    mappingType="mappings",
    prefixes={
        "chem": str(CHEM),
        "mo": str(MO),
        "map": str(MAP),
        "emmo": str(EMMO),
    },
    triples=[
        (CHEM.alloy,       MAP.mapsTo, MO.Alloy),
        (CHEM.elements,    MAP.mapsTo, EMMO.ChemicalElement),
        (CHEM.phases,      MAP.mapsTo, MO.Phase),
        (CHEM.X0,          MAP.mapsTo, MO.NominalComposition),
        (CHEM.Xp,          MAP.mapsTo, MO.PhaseComposition),
        (CHEM.volfrac,     MAP.mapsTo, EMMO.VolumeFraction),
        #(CHEM.rpart,       MAP.mapsTo, MO.ParticleSize),
        (CHEM.rpart,       MAP.mapsTo, MO.PrecipitateLength),  # XXX FIXME
        (CHEM.atvol,       MAP.mapsTo, MO.AtomicVolume),
    ],
)

chem_generate = client.create_function(
    functionType="application/vnd.dlite-generate",
    configuration={
        "driver": "template",
        "location": str(outdir / "precip.txt"),
        "options": f"template={indir / 'precip-template.txt'}",
        "datamodel": "http://onto-ns.com/meta/0.3/Chemistry",
    },
)


# Run pipeline
if False:
    pipeline = (
        tem_resource >> tem_mapping >>
        image_mapping >> image_generate >>
        settings_generate >>
        stat_convert >> stat_mapping >> stat_generate >>
        comp_resource >> comp_mapping >>
        chem_mapping >> chem_generate
    )
    pipeline.get()
else:
    pipe1 = tem_resource >> tem_mapping
    pipe2 = image_mapping >> image_generate
    pipe3 = settings_generate
    pipe4 = stat_convert >> stat_mapping >> stat_generate
    pipe5 = comp_resource >> comp_mapping
    pipe6 = chem_mapping >> chem_generate
    pipeline = pipe1 >> pipe2 >> pipe3 >> pipe4 >> pipe5 >> pipe6
    pipeline.get()
