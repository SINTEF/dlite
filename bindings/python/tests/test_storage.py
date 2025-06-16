"""Test storages."""
from pathlib import Path

import numpy as np

import dlite
from dlite.testutils import importcheck, raises
from dlite.protocol import Protocol, archive_extract

yaml = importcheck("yaml")
requests = importcheck("requests")


thisdir = Path(__file__).absolute().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"
plugindir = thisdir / "plugins"
dlite.storage_path.append(entitydir / "*.json")
dlite.python_storage_plugin_path.append(plugindir)

url = f"json://{entitydir}/MyEntity.json"


# Load metadata (i.e. an instance of meta-metadata) from url
s = dlite.Storage(url)
myentity = dlite.Instance.from_storage(
    s, "http://onto-ns.com/meta/0.1/MyEntity"
)
del s

with dlite.Storage(url) as s2:
    myentity2 = dlite.Instance.from_storage(
        s2, "http://onto-ns.com/meta/0.1/MyEntity"
    )


# Create an instance
inst = myentity(dimensions=[2, 3], id="http://data.org/my-data")
inst["a-bool-array"] = True, False
inst["a-relation"] = "subj", "pred", "obj"
inst["a-relation-array"] = [
    ("s1", "p1", "o1"),
    ("s2", "p2", "o2"),
]
assert np.all(inst["a-bool-array"] == [True, False])
assert inst["a-relation"] == dlite.Relation("subj", "pred", "obj")
assert np.all(inst["a-relation-array"] == [
    dlite.Relation("s1", "p1", "o1"),
    dlite.Relation("s2", "p2", "o2"),
])

# Test Storage.save()
with dlite.Storage("json", outdir / "test_storage_tmp.json", "mode=w") as s:
    s.save(inst)

# Test Storage.save() with options given as dict
with dlite.Storage("json", outdir / "test_storage_tmp2.json", {"mode": "w"}) as s:
    s.save(inst)

# Test query


# Test from_bytes()
mturk_bytes = '''{"9b256962-8c81-45f0-949c-c9d10b44050b": {
  "meta": "http://onto-ns.com/meta/0.1/Person",
  "dimensions": {"N": 2},
  "properties": {
    "name": "Mechanical Turk",
    "age": 83,
    "skills": ["chess", "cheating"]
  }
}}'''.encode()
mturk = dlite.Instance.from_bytes("json", mturk_bytes)
assert mturk.name == "Mechanical Turk"

# ...also test with options even though they will have not effect
mturk2 = dlite.Instance.from_bytes("json", mturk_bytes, options="mode=r")
assert mturk2.name == "Mechanical Turk"




# Test to_bytes()
assert inst.to_bytes("json").decode() == str(inst)
assert inst.to_bytes("json", options="").decode() == str(inst)
s0 = inst.to_bytes("json", options="").decode()
assert s0.startswith("{\n  \"0cd33859-")
s1 = inst.to_bytes("json", options="indent=2").decode()
assert s1.startswith("  {\n    \"0cd33859-")
s2 = inst.to_bytes("json", options="single=true").decode()
assert s2.startswith("{\n  \"uri\":")
s3 = inst.to_bytes("json", options="uri-key=true").decode()
assert s3.startswith("{\n  \"http://data.org/my-data\":")
s4 = inst.to_bytes("json", options="single=true;with-uuid=true").decode()
assert s4.startswith("{\n  \"uri\": \"http://data.org/my-data\",\n  \"uuid\":")

# FIXME: Add test for the `arrays`, `no-parent` and `compact` options.
# Should we rename `arrays` to `soft7` for consistency with the Python API?


# Test json
print("--- testing json")
myentity.save(f"json://{outdir}/test_storage_myentity.json?mode=w")
inst.save(f"json://{outdir}/test_storage_inst.json?mode=w")
rel1 = inst["a-relation"].aspreferred()
del inst
inst = dlite.Instance.from_url(
    f"json://{outdir}/test_storage_inst.json#http://data.org/my-data"
)
rel2 = inst["a-relation"].aspreferred()
assert rel1 == rel2


# Test yaml
if yaml:
    print('--- testing yaml')
    inst.save(f"yaml://{outdir}/test_storage_inst.yaml?mode=w")
    del inst
    inst = dlite.Instance.from_url(
        f"yaml://{outdir}/test_storage_inst.yaml#http://data.org/my-data"
    )

    # test help()
    expected = """\
DLite storage plugin for YAML.

Opens `location`.

Arguments:
    location: Path to YAML file.
    options: Supported options:
    - `mode`: Mode for opening.  Valid values are:
        - `a`: Open for writing, add to existing `location` (default).
        - `r`: Open existing `location` for reading.
        - `w`: Open for writing. If `location` exists, it is truncated.
    - `soft7`: Whether to save using SOFT7 format.
    - `single`: Whether to save in single-instance form.
    - `with_uuid`: Whether to include UUID when saving.
    - with_meta: Whether to always include "meta" (even for metadata)
    - with_parent: Whether to include parent info for transactions.
    - urikey: Whether the URI is the preferred keys in multi-instance
        format.
"""
    s = dlite.Storage(
        "yaml", outdir / "test_storage_inst.yaml", options="mode=a"
    )
    assert s.help().strip() == expected.strip()

    # Test delete()
    assert len(s.get_uuids()) == 1
    s.delete(inst.uri)
    assert len(s.get_uuids()) == 0

    # Test to_bytes()/from_bytes()
    data = inst.to_bytes("yaml")
    data2 = data.replace(b"my-data", b"my-data2").replace(
        dlite.get_uuid("http://data.org/my-data").encode(),
        dlite.get_uuid("http://data.org/my-data2").encode()
    )
    assert data2.startswith(b"uri: http://data.org/my-data2")

    inst2 = dlite.Instance.from_bytes("yaml", data2)
    assert inst2.uuid != inst.uuid
    assert inst2.get_hash() == inst.get_hash()

    # Test to_bytes() with options
    data3 = inst.to_bytes("yaml", options="single=false")
    assert data3.startswith(b"0cd33859-7d57-5774-8502-bd95ba61aecf")

    s.flush()  # avoid calling flush() when the interpreter is teared down


# Test rdf
try:
    print("--- testing rdf")
    inst.save(
        f"rdf:{outdir}/db.xml?mode=w;"
        "store=file;"
        f"filename={outdir}/test_storage_inst.ttl;"
        "format=turtle"
    )
except (dlite.DLiteUnsupportedError, dlite.DLiteStorageOpenError):
    print("    skipping rdf test")
else:
    # del inst
    # FIXME: read from inst.ttl not db.xml
    inst3 = dlite.Instance.from_url(
        "rdf://{outdir}/db.xml#http://data.org/my-data"
    )

    with raises(dlite.DLiteIOError):
        inst.save(
            f"rdf://{outdir}/db.xml?mode=w;"
            "store=file;"
            f"filename={outdir}/non-existing/test_storage_inst.ttl;"
            "format=turtle"
        )


# Tests for issue #587
if yaml:
    bytearr = inst.to_bytes("yaml")
    #print(bytes(bytearr).decode())

with raises(dlite.DLiteError):
    inst.to_bytes("hdf5")


s = dlite.Storage("json", f"{indir}/persons.json", "mode=r")


#assert len(list(s.instances())) == 5
#assert len(list(s.instances("xxx"))) == 0
#assert len(list(s.instances("http://onto-ns.com/meta/0.1/Person"))) == 3
#assert len(list(s.instances("http://onto-ns.com/meta/0.1/SimplePerson"))) == 2
#print(list(s.instances("http://onto-ns.com/meta/0.1/Person")))

#for inst in s.instances("http://onto-ns.com/meta/0.1/SimplePerson"):
#print("a")
#for inst in s.instances():
#    print("---")
#    #print(inst)

iter = s.instances()
#insts = list(iter)
inst = iter.next()

iter2 = s.instances()
for i in range(6):
    print(iter2.next())


# Test URL versions of dlite.Instance.save()
Blob = dlite.get_instance("http://onto-ns.com/meta/0.1/Blob")
blob = Blob([3], id="myblob")
blob.content = b'abc'
blob.save(f"file+json://{outdir}/blob1.json?mode=w")
blob.save(f"file://{outdir}/blob2.txt?driver=json;mode=w")
blob.save(f"file://{outdir}/blob3.json?mode=w")
blob.save(f"json://{outdir}/blob4.json?mode=w")
t1 = (outdir/"blob1.json").read_text()
t2 = (outdir/"blob2.txt").read_text()
t3 = (outdir/"blob3.json").read_text()
t4 = (outdir/"blob4.json").read_text()
assert t2 == t1
assert t3 == t1
assert (
    t4.replace(" ", "").replace("\n", "") ==
    t1.replace(" ", "").replace("\n", "")
)


# Test plugin that only defines to_bytes() and from_bytes()
txt = dlite.Storage.plugin_help("bufftest")
assert txt == "Test plugin that represents instances as byte-encoded json."
buf = inst.to_bytes("bufftest")
assert buf == str(inst).encode()


# Test loading instance by id
with dlite.Storage("json", indir / "persons.json"):
    ada = s.load("Ada")
