#!/usr/bin/env python
# -*- coding: utf-8 -*-
import pickle
from pathlib import Path

import numpy as np

import dlite
from dlite import Instance, Dimension, Property, Relation
from dlite.testutils import raises


thisdir = Path(__file__).absolute().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"
dlite.storage_path.append(indir / "*.json")
dlite.storage_path.append(entitydir / "*.json")

# Hide warnings
dlite.set_warnings_hide(
    True, "Warning: Could not load the following Python plugins:*"
)

# Load metadata (i.e. an instance of meta-metadata) from url
myentity = Instance.from_url(f"json://{entitydir}/MyEntity.json")
#print(myentity.uuid)

# Check some properties of the entity
assert myentity.uuid == "a0e63529-3397-5c4f-a56c-14bf07ecc219"
assert myentity.uri == "http://onto-ns.com/meta/0.1/MyEntity"
assert myentity.dimensions == {"ndimensions": 2, "nproperties": 14}
assert not myentity.is_data
assert myentity.is_meta
assert not myentity.is_metameta

# Store the entity to a new file
myentity.save(f"json://{outdir}/test_entity.json?mode=w")

# Try to overwrite without mode - should fail because metadata is immutable
with raises(dlite.DLiteStorageSaveError):
    myentity.save(f"json://{outdir}/test_entity.json")

# Create an instance of `myentity` with dimensions 2, 3
# For convinience, we give it an unique label "http://data.org/myid" that
# can be used interchangable with its uuid
inst = Instance.from_metaid(myentity.uri, [2, 3], "http://data.org/myid")
assert inst.dimensions == {"N": 2, "M": 3}
assert inst.is_data
assert not inst.is_meta
assert not inst.is_metameta

assert dlite.has_instance(inst.uuid)
assert inst.uuid in dlite.istore_get_uuids()

# Assign properties
inst["a-blob"] = bytearray(b"0123456789abcdef")
inst["a-blob"] = b"0123456789abcdef"
inst["a-blob-array"] = [[b"abcd", "00112233"], [np.int32(42), b"xyz_"]]
inst["a-blob-array"] = [[b"0123", b"4567"], [b"89ab", b"cdef"]]
inst["a-bool"] = False
inst["a-bool-array"] = True, False
inst["an-int"] = 42
inst["an-int-array"] = 1, 2, 3
inst["a-float"] = 42.3
inst["a-float64-array"] = 3.14, 5.0, 42.3
inst["a-fixstring"] = "something"
inst["a-fixstring-array"] = [["Al", "X"], ["Mg", "Si"]]
inst["a-string"] = "Hello!"
inst["a-string-array"] = [["a", "b", "c"], ["dd", "eee", "ffff"]]
inst["a-relation"] = dlite.Relation("dog", "is_a", "mammal")
inst["a-relation"] = ["dog", "is_a", "mammal"]
inst["a-relation"] = dict(s="dog", p="is_a", o="mammal")
inst["a-relation-array"] = [
    ("cheep", "is_a", "mammal"),
    dlite.Relation("cat", "is_a", "mammal"),
]


# Print the value of all properties
# for i in range(len(inst)):
#     print("prop%d:" % i, inst[i])

# String representation (as json)
# print(inst)


# Check save and load
inst.save(f"json://{outdir}/test_entity_inst.json?mode=w")
inst2 = Instance.from_url(f"json://{outdir}/test_entity_inst.json")
blob = inst2["a-blob"]

del inst2
inst2 = Instance.from_url(
    f"json://{outdir}/test_entity_inst.json?"
    "mode=r#1ad3669b-71c9-517e-be5d-449079d2b3a8"
)
assert inst2["a-blob"] == blob

del inst2
inst2 = Instance.from_location("json", outdir / "test_entity_inst.json")
assert inst2["a-blob"] == blob

del inst2
inst2 = Instance.from_location(
    "json", outdir / "test_entity_inst.json",
    id="1ad3669b-71c9-517e-be5d-449079d2b3a8"
)
assert inst2["a-blob"] == blob
del inst2

with dlite.Storage("json", outdir / "test_entity_inst.json") as s:
    inst2 = dlite.Instance.from_storage(s)
assert inst2["a-blob"] == blob
del inst2

with dlite.Storage("json", outdir / "test_entity_inst.json") as s:
    inst2 = s.load(id="1ad3669b-71c9-517e-be5d-449079d2b3a8")
assert inst2["a-blob"] == blob
del inst2

# Make sure we fail with an exception for pathetic cases
# FIXME: a part of the error message is not hidden...
with raises(dlite.DLiteError):
    Instance.from_location("json", "/", "mode=r")

with raises(dlite.DLiteStorageLoadError):
    Instance.from_location("json", "/", "mode=w")

with raises(dlite.DLiteStorageLoadError):
    Instance.from_location("json", "")

with raises(dlite.DLiteStorageLoadError):
    Instance.from_location("json", "non-existing-path...")


# Test for implicit metadata dimensions
json_repr = """
{
  "uri": "http://onto-ns.com/ex/0.1/test",
  "properties": {
    "x": {
      "type": "int"
    }
  }
}
"""
meta = dlite.Instance.from_json(json_repr)
assert meta.ndimensions == 0
assert meta.nproperties == 1
assert meta.getprop("x").type == "int64"


# Test copy
newinst = inst.copy()
assert isinstance(newinst, dlite.Instance)
assert newinst.dimensions == inst.dimensions
for newprop, prop in zip(newinst.properties, inst.properties):
    assert np.all(newprop == prop)

newmeta = inst.meta.copy()
assert isinstance(newmeta, dlite.Metadata)


# Check pickling
s = pickle.dumps(inst)
with dlite.HideDLiteWarnings():
    inst3 = pickle.loads(s)

dim = Dimension("N")

prop = Property("a", type="float")

prop2 = Property(
    "b",
    type="string10",
    shape=["I", "J", "K"],
    description="something enlightening...",
)
assert any(prop2.shape)

props = myentity["properties"]
props[0]

assert inst.meta == myentity

e = dlite.get_instance("http://onto-ns.com/meta/0.1/MyEntity")
assert e == myentity
assert e != inst

e2 = Instance.create_metadata(
    "http://onto-ns.com/meta/0.1/NewEntity",
    [Dimension("N", "Number of something")],
    [
        Property("name", type="string", description="Name of something."),
        Property("arr", type="int", shape=["N+2"], description="An array."),
        Property("v", type="double", unit="m/s", description="Velocity"),
    ],
    "Something new...",
)

e3 = Instance.create_metadata(
    "http://onto-ns.com/meta/0.1/NewEntity2",
    [],
    [
        Property("name", type="string", description="Name of something."),
        Property("arr", type="int", description="An array."),
        Property("v", type="double", unit="m/s", description="Velocity"),
    ],
    "Something new...",
)

# Test get_property_as_string() / set_property_from_string()
assert inst.get_property_as_string("an-int-array") == "[1, 2, 3]"
inst.set_property_from_string("an-int-array", "[-1, 5, 6]")
assert inst.get_property_as_string("an-int-array") == "[-1, 5, 6]"


# Test for issue #502
newinst = e(
    dimensions={"N": 2, "M": 3},
    properties={
        "a-float": 314,
        "a-string-array": [["a", "b", "c"], ["d", "e", "f"]],
    },
    id="newinst",
)
assert newinst["a-float"] == 314
assert newinst["a-string-array"].tolist() == [["a", "b", "c"], ["d", "e", "f"]]
assert newinst["an-int"] == 0

# Create a new reference
assert newinst._refcount == 1
newref = dlite.get_instance("newinst")
assert newref.uuid == newinst.uuid
assert newinst._refcount == 2


# Test save
inst.save(f"json://{outdir}/test_entity2.json?mode=w")

try:
    import yaml
except ImportError:
    pass
else:
    with dlite.HideDLiteWarnings():
        inst.save(f"yaml://{outdir}/test_entity.yaml?mode=w")


# Test metadata
assert inst.meta.dimnames() == ["N", "M"]
assert inst.meta.propnames() == [
    "a-blob",
    "a-blob-array",
    "a-bool",
    "a-bool-array",
    "an-int",
    "an-int-array",
    "a-float",
    "a-float64-array",
    "a-fixstring",
    "a-fixstring-array",
    "a-string",
    "a-string-array",
    "a-relation",
    "a-relation-array",
]


# Test property
prop = inst.meta.getprop("a-blob-array")
assert prop.name == "a-blob-array"
assert prop.type == "blob4"
assert prop.shape.tolist() == ["N", "N"]
assert prop.unit == None
assert prop.description == "A blob array."

prop = dlite.Property("newprop", "int")
prop.shape = ("a", "b", "c")
assert prop.ndims == 3
with raises(AttributeError):
    prop.ndims = 10


# Test that metadata is callable, but not instances
assert callable(inst.meta)
assert not callable(inst)


# Metadata schema
schema = dlite.get_instance(dlite.ENTITY_SCHEMA)
schema.save(f"json://{outdir}/entity_schema.json?mode=w;arrays=false")
schema.meta.save(
    f"json://{outdir}/basic_metadata_schema.json?mode=w;arrays=false"
)

mm = dlite.Instance.from_url(f"json://{outdir}/entity_schema.json")
assert mm.uri == dlite.ENTITY_SCHEMA


# Test loading invalid json input
with dlite.errctl(hide=True):
    Invalid1 = dlite.get_instance("http://onto-ns.com/meta/0.1/Invalid1")
with raises(dlite.DLiteMissingInstanceError, dlite.DLiteSyntaxError):
    invalid1 = Invalid1([2], properties={"name": "a", "f": [3.14, 2.72]})

# For issue #686
Invalid2 = dlite.get_instance("http://onto-ns.com/meta/0.1/Invalid2")
with raises(dlite.DLiteMissingInstanceError, dlite.DLiteSyntaxError):
    invalid2 = Invalid2([2])


# For issue #691
with raises(dlite.DLiteStorageOpenError, dlite.DLiteUnknownError):
    dlite.Instance.from_location("json", entitydir / "Invalid3.json")

with raises(dlite.DLiteStorageOpenError, dlite.DLiteUnknownError):
    dlite.Instance.from_location("json", entitydir / "Invalid4.json")

with raises(dlite.DLiteStorageOpenError, dlite.DLiteUnknownError):
    dlite.Instance.from_location("json", entitydir / "Invalid5.json")

with raises(dlite.DLiteStorageOpenError, dlite.DLiteUnknownError):
    dlite.Instance.from_location("json", entitydir / "Invalid6.json")

with raises(dlite.DLiteStorageOpenError, dlite.DLiteUnknownError):
    dlite.Instance.from_location("json", entitydir / "Invalid7.json")


# For issue #702
PersonOld = dlite.get_instance("http://onto-ns.com/meta/0.1/PersonOld")
PersonNew = dlite.get_instance("http://onto-ns.com/meta/0.1/PersonNew")
assert PersonOld.props == PersonNew.props
assert PersonOld.dimnames() == PersonNew.dimnames()


# For issue #743
Item = dlite.get_instance("http://onto-ns.com/meta/0.1/Item")
item = Item(dimensions={"nf": 3}, properties={"name": "A", "f": [1, 2, 3]})
try:
    import pint
except ModuleNotFoundError:
    pass
else:
    item.q.f = pint.Quantity([0.1, 0.2, 0.3], "kHz")
    assert item.f.tolist() == [100., 200., 300.]
    item.q.f = ([0.5, 0.2, 0.3], "kHz")
    assert item.f.tolist() == [500., 200., 300.]

    item.set_quantity("f", [0, 3600, 3.6], "hour**-1")
    assert item.f.tolist() == [0., 1., 0.001]
    assert item.q.f.m.tolist() == [0., 1., 0.001]
    assert item.q.f.to("1/hour").m.tolist() == [0, 3600, 3.6]


# For issue #750 - test instance_cast()
with raises(dlite.DLiteTypeError):
    dlite.instance_cast(inst, dlite.Metadata)
castinst = dlite.instance_cast(inst.meta, dlite.Instance)
assert type(castinst) == dlite.Instance
assert type(dlite.instance_cast(castinst)) == dlite.Metadata


# Test storage query
uuids = {
    '850637b9-1d21-573c-91b6-477530e4bf58',
    '020e411b-f349-5689-8657-f82b709369c3',
    '570611f5-96b3-5b0d-90ad-f3a4c19a78b2',
    '5e378ac7-83c9-5d77-ab20-b5bb32c695da',
    'e5efe084-27f2-5fec-9b1c-fa1a692e1434',
}
with dlite.Storage("json", indir / "test_ref_type.json") as s:
    assert set(s.get_uuids()) == uuids
    assert set(s.get_uuids("http://onto-ns.com/meta/0.3/EntitySchema")) == uuids
    assert s.get_uuids("xxx") == []
assert set(
    dlite.Instance.get_uuids("json", indir / "test_ref_type.json")
) == uuids
assert set(
    dlite.Instance.get_uuids(
        "json", indir / "test_ref_type.json",
        pattern="http://onto-ns.com/meta/0.3/EntitySchema",
    )
) == uuids
assert dlite.Instance.get_uuids(
    "json", indir / "test_ref_type.json",
    pattern="xxx",
) == []
