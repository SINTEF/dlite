#!/usr/bin/env python
from pathlib import Path

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

# Test Storage.save()
with dlite.Storage("json", outdir / "test_storage_tmp.json", "mode=w") as s:
    s.save(inst)

# Test query


# Test json
print("--- testing json")
myentity.save(f"json://{outdir}/test_storage_myentity.json?mode=w")
inst.save(f"json://{outdir}/test_storage_inst.json?mode=w")
del inst
inst = dlite.Instance.from_url(f"json://{outdir}/test_storage_inst.json#my-data")


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
    data2 = data.replace(b"uri: my-data", b"uri: my-data2")
    inst2 = dlite.Instance.from_bytes("yaml", data2)
    assert inst2.uuid != inst.uuid
    assert inst2.get_hash() == inst.get_hash()

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

    with raises(dlite.DLiteIOError):
        inst.save(
            f"rdf://{outdir}/db.xml?mode=w;"
            "store=file;"
            f"filename={outdir}/non-existing/test_storage_inst.ttl;"
            "format=turtle"
        )
except dlite.DLiteUnsupportedError:
    print("    skipping rdf test")
else:
    # del inst
    # FIXME: read from inst.ttl not db.xml
    inst3 = dlite.Instance.from_url("rdf://{outdir}/db.xml#my-data")




# Tests for issue #587
if HAVE_YAML:
    bytearr = inst.to_bytes("yaml")
    #print(bytes(bytearr).decode())

with raises(dlite.DLiteError):
    inst.to_bytes("json")


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
