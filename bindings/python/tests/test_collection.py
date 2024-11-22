#!/usr/bin/env python
# -*- coding: utf-8 -*-
from pathlib import Path

import dlite
from dlite.testutils import raises


thisdir = Path(__file__).resolve().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"


# Create collection
coll = dlite.Collection("mycoll")

# Add relations
coll.add_relation("cat", "is-a", "animal")
coll.add_relation("dog", "is-a", "animal")
rel = coll.get_first_relation("dog")
assert rel.s == "dog"
rel = coll.get_first_relation(p="is-a")
assert rel.s == "cat"
assert coll.nrelations == 2
rel = coll.get_first_relation(s="no-such-subject")
assert rel is None

# Create instances
url = f"json://{entitydir}/MyEntity.json?mode=r"
e = dlite.Instance.from_url(url)
inst1 = dlite.Instance.from_metaid(e.uri, [3, 2])
inst2 = dlite.Instance.from_metaid(e.uri, (3, 4), "myinst")

# Add instances
coll.add("inst1", inst1)
coll.add("inst2", inst2)
assert len(coll) == 2
assert coll.has("inst1")
assert not coll.has("inst3")
assert coll.has_id(inst2.uuid)
assert coll.has_id(inst2.uri)
assert not coll.has_id("non-existing-id")

# Save
with dlite.Storage("json", outdir / "coll0.json", "mode=w") as s:
    coll.save(s)
coll.save("json", outdir / "coll1.json", "mode=w")
coll.save(f"json://{outdir}/coll2.json?mode=w")
coll.save(f"json://{outdir}/coll3.json?mode=w", include_instances=False)
data = []
for i in range(3):
    with open(outdir / f"coll{i}.json") as f:
        data.append(f.read())
assert data[1] == data[0]
assert data[2] == data[0]

# Load
with dlite.Storage("json", outdir / "coll0.json", "mode=r") as s:
    coll0 = dlite.Collection.load(s, id="mycoll")
coll1 = dlite.Collection.load(
    "json", outdir / "coll1.json", "mode=r", id=coll.uuid
)
coll2 = dlite.Collection.load(f"json://{outdir}/coll2.json?mode=r#mycoll")
assert coll0 == coll
assert coll1 == coll
assert coll2 == coll

# Check refcount
assert coll._refcount == 4
del coll0
assert coll._refcount == 3

# Remove relation
assert coll.nrelations == 8
coll.remove_relations("cat")
assert coll.nrelations == 7
rel = coll.get_first_relation("dog")
assert rel.s == "dog"

# Remove instance
assert len(coll) == 2
coll.remove("inst2")
assert len(coll) == 1
inst1b = coll.get("inst1")
assert inst1b == inst1
assert inst1b != inst2

# Cannot add an instance with an existing label
try:
    coll.add("inst1", inst2)
except dlite.DLiteError:
    pass
else:
    raise RuntimeError("should not be able to replace an existing instance")

coll.add("inst1", inst2, force=True)  # forced replacement
assert coll.get("inst1") == inst2
coll.add("inst1", inst1, force=True)  # revert
assert coll.get("inst1") == inst1

# Test convinience functions
i1 = coll.get_id(inst1.uuid)
assert i1 == inst1

assert coll.has("inst1") is True
assert coll.has("inst2") is False
assert coll.has("animal") is False

rel = coll.get_first_relation()
assert rel.s == "dog"
assert rel.p == "is-a"
assert rel.o == "animal"
rel = coll.get_first_relation(p="_has-meta")
assert rel.s == "inst1"
assert rel.p == "_has-meta"
assert rel.o == "http://onto-ns.com/meta/0.1/MyEntity"

(i1,) = coll.get_instances()
assert i1 == inst1


# We have no collections in the collection
assert not list(coll.get_instances(metaid=dlite.COLLECTION_ENTITY))

(i1,) = coll.get_instances(metaid="http://onto-ns.com/meta/0.1/MyEntity")
assert i1 == inst1

(label1,) = coll.get_labels()
assert label1 == "inst1"

rels = list(coll.get_relations())
assert len(rels) == 4
rels = list(coll.get_relations(p="_is-a"))
assert len(rels) == 1
rels = list(coll.get_relations(p="_xxx"))
assert len(rels) == 0

assert list(coll.get_subjects()) == ["dog", "inst1", "inst1", "inst1"]
assert list(coll.get_predicates()) == [
    "is-a",
    "_is-a",
    "_has-uuid",
    "_has-meta",
]
assert list(coll.get_objects()) == [
    "animal",
    "Instance",
    inst1.uuid,
    inst1.meta.uri,
]


# Test that coll.copy() is a collection
newcoll = coll.copy()
assert isinstance(newcoll, dlite.Collection)

# Test Collection.get() returns the right Instance subclass
coll.add("newcoll", newcoll)
coll.add("meta", newcoll.meta)
assert isinstance(coll["inst1"], dlite.Instance)
assert isinstance(coll["newcoll"], dlite.Collection)
assert isinstance(coll["meta"], dlite.Metadata)


# Test Collection.value()
assert coll.value(s="dog", p="is-a") == "animal"
assert coll.value(p="_is-a", o="Instance", any=1) == "inst1"
assert coll.value(p="is-a", o="x", default="y", any=1) == "y"
assert coll.value(s="meta", p="_has-uuid", d="xsd:anyURI") == (
    "96f31fc3-3838-5cb8-8d90-eddee6ff59ca"
)

with raises(dlite.DLiteTypeError):
    coll.value(s="dog", p="is-a", o="animal")

with raises(dlite.DLiteTypeError):
    coll.value()

with raises(dlite.DLiteLookupError):
    coll.value(s="dog", p="x")

with raises(dlite.DLiteLookupError):
    coll.value(p="_is-a", o="Instance")

with raises(dlite.DLiteLookupError):
    coll.value(s="meta", p="_has-uuid", d="xsd:int")


# String representation
s = str(coll)
print(s)
