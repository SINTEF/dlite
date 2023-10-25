#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite

try:
    import pytest

    HAVE_PYTEST = True
except ModuleNotFoundError:
    HAVE_PYTEST = False

try:
    import yaml
    HAVE_YAML = True
except ModuleNotFoundError:
    HAVE_YAML = False


thisdir = os.path.abspath(os.path.dirname(__file__))
outdir = f"{thisdir}/output"

url = "json://" + thisdir + "/MyEntity.json"


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
with dlite.Storage("json", f"{outdir}/tmp.json", "mode=w") as s:
    s.save(inst)

# Test query


# Test json
print("--- testing json")
myentity.save(f"json://{outdir}/myentity.json?mode=w")
inst.save(f"json://{outdir}/inst.json?mode=w")
del inst
inst = dlite.Instance.from_url(f"json://{outdir}/inst.json#my-data")


# Test yaml
if HAVE_YAML:
    print('--- testing yaml')
    inst.save(f"yaml://{outdir}/inst.yaml?mode=w")
    del inst
    inst = dlite.Instance.from_url(f"yaml://{outdir}/inst.yaml#my-data")

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
    s = dlite.Storage("yaml", f"{outdir}/inst.yaml", options="mode=a")
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
    inst.save("rdf:db.xml?mode=w;store=file;filename=inst.ttl;format=turtle")
except dlite.DLiteError:
    print("    skipping rdf test")
else:
    # del inst
    # FIXME: read from inst.ttl not db.xml
    inst3 = dlite.Instance.from_url("rdf://db.xml#my-data")


# Tests for issue #587
if HAVE_YAML:
    bytearr = inst.to_bytes("yaml")
    #print(bytes(bytearr).decode())
if HAVE_PYTEST:
    with pytest.raises(dlite.DLiteError):
        inst.to_bytes("json")
    dlite.errclr()


dlite.storage_path.append(thisdir)
s = dlite.Storage("json", f"{thisdir}/persons.json", "mode=r")


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
