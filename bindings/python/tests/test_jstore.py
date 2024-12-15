"""Test JStore."""
from pathlib import Path

import dlite
from dlite.testutils import raises


thisdir = Path(__file__).resolve().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"

dlite.storage_path.append(entitydir)


# Test format_dict(), arg: soft7
D1 = {  # soft7 representation
    "uri": "http://onto-ns.com/meta/ex/0.2/Test",
    "dimensions": {"n": "number of something"},
    "properties": {
        "a": {"type": "string"},
        "b": {"type": "float64", "shape": ["n"]},
    },
}
D2 = {  # old (array) representation
    "uri": "http://onto-ns.com/meta/ex/0.2/Test",
    "dimensions": [{"name": "n", "description": "number of something"}],
    "properties": [
        {"name": "a", "type": "string"},
        {"name": "b", "type": "float64", "shape": ["n"]},
    ],
}
assert dlite.format_dict(D1, soft7=True) == D1
assert dlite.format_dict(D1, soft7=False) == D2
assert dlite.format_dict(D2, soft7=True) == D1
assert dlite.format_dict(D2, soft7=False) == D2

# soft7 representation.  This is identical to the old representation for
# data instances
d1 = {
    "uuid": "d6a1c1db-44b6-5b87-b815-83f1127395b6",
    "meta": "http://onto-ns.com/meta/ex/0.2/Test",
    "dimensions": {"n": 3},
    "properties": {
        "a": "hello",
        "b": [1.1, 2.2, 3.3],
    },
}
d2 = {  # old representation
    "uuid": "d6a1c1db-44b6-5b87-b815-83f1127395b6",
    "meta": "http://onto-ns.com/meta/ex/0.2/Test",
    "dimensions": {"n": 3},
    "properties": {
        "a": "hello",
        "b": [1.1, 2.2, 3.3],
    },
}
d3 = {
    "uuid": "d6a1c1db-44b6-5b87-b815-83f1127395b6",
    "meta": "http://onto-ns.com/meta/ex/0.2/Test",
    "dimensions": [3],
    "properties": {
        "a": "hello",
        "b": [1.1, 2.2, 3.3],
    },
}
assert dlite.format_dict(d1, soft7=True, single=True) == d1
assert dlite.format_dict(d1, soft7=False, single=True) == d2
assert dlite.format_dict(d2, soft7=True, single=True) == d1
assert dlite.format_dict(d2, soft7=False, single=True) == d2

# Test format_dict(), dimension as list of numbers - need metadata
js = dlite.JStore()
js.load_dict(D1)
with dlite.HideDLiteWarnings():
    meta = js.get()
assert dlite.format_dict(d3, soft7=True, single=True) == d1
assert dlite.format_dict(d3, soft7=False, single=True) == d2

# Test format_dict(), arg: single
uuid_D1 = dlite.get_uuid(D1["uri"])
assert dlite.format_dict(D1) == D1
assert dlite.format_dict(D1, single=True) == D1
assert dlite.format_dict(D1, single=False) == {uuid_D1: D1}

d1_nouuid = d1.copy()
del d1_nouuid["uuid"]
d1_multi = {d1["uuid"]: d1_nouuid}
assert dlite.format_dict(d1, single=None) == d1_multi
assert dlite.format_dict(d1, single=True) == d1
assert dlite.format_dict(d1, single=False) == d1_multi

# Test format_dict(), arg: with_uuid
assert dlite.format_dict(d1, single=None, with_uuid=True) == {d1["uuid"]: d1}
assert dlite.format_dict(d1, single=True, with_uuid=True) == d1
assert dlite.format_dict(d1, single=False, with_uuid=True) == {d1["uuid"]: d1}

assert dlite.format_dict(d1, single=None, with_uuid=False) == d1_multi
assert dlite.format_dict(d1, single=True, with_uuid=False) == d1_nouuid
assert dlite.format_dict(d1, single=False, with_uuid=False) == d1_multi

# Test format_dict(), arg: with_meta
D1_meta = D1.copy()
D1_meta["meta"] = dlite.ENTITY_SCHEMA
assert dlite.format_dict(D1, single=True, with_meta=True) == D1_meta
assert dlite.format_dict(D1, single=True, with_meta=False) == D1
assert dlite.format_dict(D1, single=False, with_meta=True) == {uuid_D1: D1_meta}
assert dlite.format_dict(D1, single=False, with_meta=False) == {uuid_D1: D1}

d1_nometa = d1.copy()
del d1_nometa["meta"]
assert dlite.format_dict(d1, single=True, with_meta=True) == d1
assert dlite.format_dict(d1, single=True, with_meta=False) == d1
assert dlite.format_dict(d1, single=False, with_meta=True) == d1_multi
assert dlite.format_dict(d1, single=False, with_meta=False) == d1_multi

# Test format_dict(), arg: urikey
assert dlite.format_dict(D1, single=True, urikey=True) == D1
assert dlite.format_dict(D1, single=False, urikey=True) == {D1["uri"]: D1}
assert dlite.format_dict(D1, single=False, urikey=False) == {uuid_D1: D1}
assert dlite.format_dict(d1, single=False, urikey=True) == d1_multi
assert dlite.format_dict(d1, single=False, urikey=False) == d1_multi

# Test format_dict(), arg: id
d4 = {"inst1": d1_nouuid}
d5 = {
    "inst1": d1,
    "inst2": d1_nouuid,
    "inst3": d1_nouuid,
}
d1_uri = d1.copy()
d1_uri["uri"] = "inst1"
d1_uri_nouuid = d1_uri.copy()
del d1_uri_nouuid["uuid"]
d1_uri_multi = {d1["uuid"]: d1_uri_nouuid}
assert dlite.format_dict(d4, id="inst1", single=True) == d1_uri
assert dlite.format_dict(d4, id="inst1", single=None) == d1_uri_multi
assert dlite.format_dict(d4, id="inst1", single=False) == d1_uri_multi
assert dlite.format_dict(d4) == {d1["uuid"]: d1_uri}
with raises(dlite.DLiteLookupError):
    dlite.format_dict(d4, id="noexisting")

# FIXME - make sure that the uri is included
assert dlite.format_dict(d5, id="inst1", single=True) == d1
assert dlite.format_dict(d5, id="inst1", single=None) == d1_multi
assert dlite.format_dict(d5, id="inst1", single=False) == d1_multi
# assert dlite.format_dict(d5, id="inst1", single=True) == d1_uri
# assert dlite.format_dict(d5, id="inst1", single=None) == d1_uri_multi
# assert dlite.format_dict(d5, id="inst1", single=False) == d1_uri_multi
# assert len(dlite.format_dict(d5)) == 3
with raises(dlite.DLiteLookupError):
    dlite.format_dict(d5, id="noexisting")
with raises(dlite.DLiteLookupError):
    dlite.format_dict(d5, single=True)


# Test JStore
js = dlite.JStore()

with raises(dlite.DLiteLookupError):
    js.get()

js.load_file(indir / "blob.json")
assert len(js) == 1
inst = js.get()
key = next(js.get_ids())

js.load_file(indir / "persons.json")
assert len(js) == 6

js.remove(key)
assert len(js) == 5

with raises(dlite.DLiteLookupError):
    js.get()

metaid = "http://onto-ns.com/meta/0.1/Person"
jon = js.get_dict(id="028217b9-2f64-581d-9712-a5b67251bfec", single=None)
assert "028217b9-2f64-581d-9712-a5b67251bfec" in jon
assert jon["028217b9-2f64-581d-9712-a5b67251bfec"]["meta"] == metaid

cleo = js.get_dict(id="Cleopatra", single=True)
assert cleo["uuid"] == dlite.get_uuid("Cleopatra")
assert cleo["meta"] == "http://onto-ns.com/meta/0.1/SimplePerson"

js.load_dict(D1)
js.load_dict(d1)
assert len(js) == 7

# Reloading existing will just replace
js.load_dict(d1)
assert len(js) == 7


js2 = dlite.JStore()
js2.load_dict(D1)
js2.load_dict(d1)
assert len(js2) == 2

dct = js2.get_dict()
assert len(dct) == 2


key1 = next(js.get_ids())
# d1 = js.get_dict(key1)
s1 = js.get_json(key1)
with dlite.HideDLiteWarnings():
    inst1 = js.get(key1)
