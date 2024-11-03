import os
from pathlib import Path

import dlite

try:
    import bson
except ImportError:
    HAVE_BSON = False
else:
    HAVE_BSON = True

try:
    import yaml as pyyaml
except ImportError:
    HAVE_YAML = False
else:
    HAVE_YAML = True

try:
    import rdflib
    from rdflib.util import guess_format
    from rdflib.compare import to_isomorphic
except ImportError:
    HAVE_RDF = False
else:
    HAVE_RDF = True


# thisdir = os.path.abspath(os.path.dirname(__file__))
thisdir = Path(__file__).resolve().parent
outdir = thisdir / "output"
entitydir = thisdir / "entities"


def equal_rdf_files(path1, path2):
    """Help function, returning true if the rdf graphs stored in `path1`
    and `path2` are equal."""
    fmt1 = guess_format(path1)
    fmt2 = guess_format(path2)
    g1 = rdflib.Graph()
    g2 = rdflib.Graph()
    g1.parse(str(path1), format=fmt1)
    g2.parse(str(path2), format=fmt2)
    iso1 = to_isomorphic(g1)
    iso2 = to_isomorphic(g2)
    return iso1 == iso2


# Test JSON
Person = dlite.Instance.from_url(f"json://{entitydir}/Person.json")
person = Person(dims=[2])
person.name = "Ada"
person.age = 12.5
person.skills = ["skiing", "jumping"]

print("=== saving...")
with dlite.Storage("json", outdir / "test_python_storage.json", "mode=w") as s:
    s.save(person)

print("=== loading...", person.uuid)
with dlite.Storage("json", outdir / "test_python_storage.json", "mode=r") as s:
    inst = s.load(id=person.uuid)


person2 = Person(dimensions=[3])
person2.name = "Berry"
person2.age = 24.3
person2.skills = ["eating", "sleeping", "reading"]
with dlite.Storage(f"json://{outdir}/test_python_storage.json") as s:
    s.save(person2)


s = dlite.Storage(f"json://{outdir}/test_python_storage.json")
uuids = s.get_uuids()
del s
del uuids

# =====================================================================
# Test the BSON and YAML Python plugins

input_dir = thisdir.parent.parent.parent / "storages/python/tests-python/input"

if HAVE_BSON:
    # Test BSON
    print("\n\n=== Test BSON plugin ===")
    meta_infile = input_dir / "test_meta.bson"
    meta_outfile = outdir / "test_meta_save.bson"
    data_infile = input_dir / "test_data.bson"
    data_outfile = outdir / "test_data_save.bson"

    print("Test loading metadata...")
    with dlite.Storage("bson", meta_infile, "mode=r") as s:
        meta = s.load("2b10c236-eb00-541a-901c-046c202e52fa")
    print("...Loading metadata ok!")

    print("Test saving metadata...")
    with dlite.Storage("bson", meta_outfile, "mode=w") as s:
        s.save(meta)
    with dlite.Storage("bson", meta_outfile, "mode=r") as s:
        inst2 = s.load("2b10c236-eb00-541a-901c-046c202e52fa")
    if meta == inst2:
        print("...Saving metadata ok!")
    else:
        raise ValueError("...Saving metadata failed!")
    os.remove(meta_outfile)
    del meta, inst2

    print("Test loading data...")
    with dlite.Storage("bson", data_infile, "mode=r") as s:
        inst1 = s.load("204b05b2-4c89-43f4-93db-fd1cb70f54ef")
        inst2 = s.load("e076a856-e36e-5335-967e-2f2fd153c17d")
    print("...Loading data ok!")

    print("Test saving data...")
    with dlite.Storage("bson", data_outfile, "mode=w") as s:
        s.save(inst1)
        s.save(inst2)
    with dlite.Storage("bson", data_outfile, "mode=r") as s:
        inst3 = s.load("204b05b2-4c89-43f4-93db-fd1cb70f54ef")
        inst4 = s.load("e076a856-e36e-5335-967e-2f2fd153c17d")
    if inst1 == inst3 and inst2 == inst4:
        print("...Saving data ok!")
    else:
        raise ValueError("...Saving data failed!")
    os.remove(data_outfile)
    # del inst1, inst2, inst3, inst4
else:
    print("Skip testing BSON plugin - bson not installed")


if HAVE_YAML:
    # Test YAML
    print("\n\n=== Test YAML plugin ===")
    meta_infile = input_dir / "test_meta_soft7.yaml"
    meta_outfile = outdir / "test_meta_save.yaml"
    data_infile = input_dir / "test_data.yaml"
    data_outfile = outdir / "test_data_save.yaml"

    print("Test loading metadata...")
    with dlite.Storage("yaml", meta_infile, "mode=r") as s:
        # meta = s.load('d9910bde-6028-524c-9e0f-e8f0db734bc8')
        meta = s.load("http://onto-ns.com/meta/0.1/TestEntity")
    print("...Loading metadata ok!")

    print("Test saving metadata...")
    with dlite.Storage(
            "yaml",
            meta_outfile,
            "mode=w;uuid=false;single=false;with_uuid=false;with_meta=true"
    ) as s:
        s.save(meta)
    with open(meta_infile, "r") as f:
        d1 = pyyaml.safe_load(f)
    with open(meta_outfile, "r") as f:
        d2 = pyyaml.safe_load(f)
    assert d1 == d2
    print("...Saving metadata ok!")
    os.remove(meta_outfile)

    print("Test loading data...")
    with dlite.Storage("yaml", data_infile, "mode=r") as s:
        inst1 = s.load("52522ba5-6bfe-4a64-992d-e9ec4080fbac")
        inst2 = s.load("2f8ba28c-add6-5718-a03c-ea46961d6ca7")
    print("...Loading data ok!")

    print("Test saving data...")
    with dlite.Storage("yaml", data_outfile, "mode=w;with_uuid=false") as s:
        s.save(inst1)
        s.save(inst2)
    with open(data_infile, "r") as f:
        d1 = pyyaml.safe_load(f)
    with open(data_outfile, "r") as f:
        d2 = pyyaml.safe_load(f)
    assert d1 == d2
    print("...Saving data ok!")
    os.remove(data_outfile)
    del inst1, inst2
else:
    print("Skip testing YAML plugin - PyYAML not installed")


if HAVE_RDF:
    # Test RDF
    print("\n\n=== Test RDF plugin ===")
    meta_infile = input_dir / "test_meta.ttl"
    meta_outfile = outdir / "test_meta_save.ttl"
    data_infile = input_dir / "test_data.ttl"
    data_outfile = outdir / "test_data_save.ttl"

    print("Test loading metadata...")
    with dlite.Storage("pyrdf", meta_infile, "mode=r") as s:
        meta = s.load("http://onto-ns.com/meta/0.2/myentity")
    print("...Loading metadata ok!")

    print("Test saving metadata...")
    with dlite.Storage("pyrdf", meta_outfile, "mode=w") as s:
        s.save(meta)
    assert equal_rdf_files(meta_infile, meta_outfile)
    print("...Saving metadata ok!")
    os.remove(meta_outfile)

    from dlite.rdf import DM, PUBLIC_ID, from_rdf
    import rdflib
    from rdflib import URIRef, Literal

    print("Test loading data...")
    with dlite.Storage("pyrdf", data_infile, "mode=r") as s:
        inst1 = s.load("inst_with_uri")
        # inst1 = s.load('2713c649-e9b1-5f5e-8abb-8a6e3e610a61')
        inst2 = s.load("67128279-c3fa-4483-8842-eb571f94a1ae")
    print("...Loading data ok!")

    print("Test saving data...")
    with dlite.Storage("pyrdf", data_outfile, "mode=w") as s:
        s.save(inst1)
        s.save(inst2)
    # FIXME: seems we have a bug in rdflib
    # assert equal_rdf_files(data_infile, data_outfile)
    print("...Saving data ok!")
    os.remove(data_outfile)
    del inst1, inst2
else:
    print("Skip testing RDF plugin - rdflib not installed")
