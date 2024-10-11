#!/usr/bin/env python
from pathlib import Path

import numpy as np

import dlite
from dlite.testutils import raises

try:
    import yaml
    HAVE_YAML = True
except ModuleNotFoundError:
    HAVE_YAML = False


thisdir = Path(__file__).absolute().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"
dlite.storage_path.append(entitydir / "*.json")

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
inst = myentity(dimensions=[2, 3], id="my-data")
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
assert s0.startswith("{\n  \"ce592ff9-")
s1 = inst.to_bytes("json", options="indent=2").decode()
assert s1.startswith("  {\n    \"ce592ff9-")
s2 = inst.to_bytes("json", options="single=true").decode()
assert s2.startswith("{\n  \"uri\":")
s3 = inst.to_bytes("json", options="uri-key=true").decode()
assert s3.startswith("{\n  \"my-data\":")
s4 = inst.to_bytes("json", options="single=true;with-uuid=true").decode()
assert s4.startswith("{\n  \"uri\": \"my-data\",\n  \"uuid\":")

# FIXME: Add test for the `arrays`, `no-parent` and `compact` options.
# Should we rename `arrays` to `soft7` for consistency with the Python API?

with raises(dlite.DLiteValueError):
    inst.to_bytes("json", options="invalid-opt=").decode()


# Test json
print("--- testing json")
myentity.save(f"json://{outdir}/test_storage_myentity.json?mode=w")
inst.save(f"json://{outdir}/test_storage_inst.json?mode=w")
rel1 = inst["a-relation"].aspreferred()
del inst
inst = dlite.Instance.from_url(f"json://{outdir}/test_storage_inst.json#my-data")
rel2 = inst["a-relation"].aspreferred()
assert rel1 == rel2


# Test yaml
if HAVE_YAML:
    print('--- testing yaml')
    inst.save(f"yaml://{outdir}/test_storage_inst.yaml?mode=w")
    del inst
    inst = dlite.Instance.from_url(f"yaml://{outdir}/test_storage_inst.yaml#my-data")

    # test help()
    expected = """\
DLite storage plugin for YAML.

Opens `location`.

        Arguments:
            location: Path to YAML file.
            options: Supported options:
            - `mode`: Mode for opening.  Valid values are:
                - `a`: Append to existing file or create new file (default).
                - `r`: Open existing file for read-only.
                - `w`: Truncate existing file or create new file.
            - `soft7`: Whether to save using SOFT7 format.
            - `single`: Whether the input is assumed to be in single-entity form.
              If "auto" (default) the form will be inferred automatically.
            - `with_uuid`: Whether to include UUID when saving.
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
    data2 = data.replace(b"uri: my-data", b"uri: my-data2").replace(
        dlite.get_uuid("my-data").encode(),
        dlite.get_uuid("my-data2").encode()
    )
    assert data2.startswith(b"uri: my-data2")

    inst2 = dlite.Instance.from_bytes("yaml", data2)
    assert inst2.uuid != inst.uuid
    assert inst2.get_hash() == inst.get_hash()

    # Test to_bytes() with options
    data3 = inst.to_bytes("yaml", options="single=false")
    assert data3.startswith(b"ce592ff9-a7dc-548a-bbe5-3c53800afaf3")

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
    inst3 = dlite.Instance.from_url("rdf://{outdir}/db.xml#my-data")

    with raises(dlite.DLiteIOError):
        inst.save(
            f"rdf://{outdir}/db.xml?mode=w;"
            "store=file;"
            f"filename={outdir}/non-existing/test_storage_inst.ttl;"
            "format=turtle"
        )



# Tests for issue #587
if HAVE_YAML:
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

print("==========")

for i in range(5):
    print(iter.next())
