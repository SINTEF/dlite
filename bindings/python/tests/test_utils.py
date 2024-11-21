#!/usr/bin/env python
import os
import json
from pathlib import Path

import dlite
from dlite.utils import (
    DictStore,
    instance_from_dict,
    to_metadata,
    infer_dimensions,
    HAVE_DATACLASSES,
    HAVE_PYDANTIC,
)


thisdir = Path(__file__).absolute().parent
entitydir = thisdir / "entities"
dlite.storage_path.append(entitydir / "*.json")

with open(entitydir / "Person.json", "rt") as f:
    d = json.load(f)
Person = dlite.utils.instance_from_dict(d)

person = Person([2])
person.name = "Ada"
person.age = 12.5
person.skills = ["skiing", "jumping"]

d1 = person.asdict()
inst1 = instance_from_dict(d1)
assert inst1.uuid == person.uuid
assert inst1.meta.uuid == Person.uuid
assert inst1.name == "Ada"
assert inst1.age == 12.5
assert all(inst1.skills == ["skiing", "jumping"])

d2 = Person.asdict()
inst2 = instance_from_dict(d2)
assert inst2.uuid == Person.uuid


dlite.storage_path.append(thisdir / "*.json")
d = {
    "uuid": "7ee0f569-1355-4eed-a2f7-0fc31378d56c",
    "meta": "http://onto-ns.com/meta/0.1/MyEntity",
    "dimensions": {"N": 2, "M": 3},
    "properties": {
        "a-blob": "00112233445566778899aabbccddeeff",
        "a-blob-array": [[b"abcd", b"efgh"], [b"ijkl", b"mnop"]],
        "a-bool": True,
        "a-bool-array": [False, True],
        "an-int": 42,
        "an-int-array": [-1, -2, -3],
        "a-float": 3.14,
        "a-float64-array": [0.0, 1.6022e-19, 6.022e23],
        "a-fixstring": "fix",
        "a-fixstring-array": [["one", "two"], ["three", "four"]],
        "a-string": None,
        "a-string-array": [[None, "a string", None], [None, None, None]],
        "a-relation": {"s": "a-subject", "p": "a-predicate", "o": "a-object"},
        "a-relation-array": [
            {"s": "a1", "p": "b1", "o": "c1"},
            {"s": "a2", "p": "b2", "o": "c2"},
        ],
    },
}
inst = instance_from_dict(d)
print(inst)


Person = dlite.Instance.from_url(f"json://{entitydir}/Person.json")
person = Person([2])
person.name = "Ada"
person.age = 12.5
person.skills = ["skiing", "jumping"]

d1 = person.asdict()
inst1 = instance_from_dict(d1)

d2 = Person.asdict()
inst2 = instance_from_dict(d2)


if HAVE_DATACLASSES:
    from dlite.utils import get_dataclass_entity_schema

    EntitySchema = get_dataclass_entity_schema()
    AtomsEntity = EntitySchema(
        uri="http://onto-ns.com/meta/0.1/AtomsEntity",
        description="A structure consisting of a set of atoms.",
        dimensions={
            "natoms": "Number of atoms.",
            "ncoords": "Number of coordinates.  Always three.",
        },
        properties={
            "symbols": {
                "type": "string",
                "shape": ["natoms"],
                "description": "Chemical symbol of each atom.",
            },
            "positions": {
                "type": "float",
                "shape": ["natoms", "ncoords"],
                "unit": "Å",
                "description": "Position of each atom.",
            },
        },
    )
    Atoms = to_metadata(AtomsEntity)
    assert Atoms.is_meta
    assert Atoms.meta.uri == dlite.ENTITY_SCHEMA


if HAVE_PYDANTIC:
    from dlite.utils import get_pydantic_entity_schema

    PydanticEntitySchema = get_pydantic_entity_schema()
    AtomsEntity2 = PydanticEntitySchema(
        uri="http://onto-ns.com/meta/0.1/AtomsEntity",
        description="A structure consisting of a set of atoms.",
        dimensions={
            "natoms": "Number of atoms.",
            "ncoords": "Number of coordinates.  Always three.",
        },
        properties={
            "symbols": {
                "type": "string",
                "shape": ["natoms"],
                "description": "Chemical symbol of each atom.",
            },
            "positions": {
                "type": "float",
                "shape": ["natoms", "ncoords"],
                "unit": "Å",
                "description": "Position of each atom.",
            },
        },
    )
    Atoms2 = to_metadata(AtomsEntity2)
    assert Atoms2.is_meta
    assert Atoms2.meta.uri == dlite.ENTITY_SCHEMA


# Test infer_dimensions()
# TODO - test also exceptions
dims = infer_dimensions(
    meta=inst.meta,
    values={"a-string-array": [("a", "b"), ("c", "d"), ("e", "f")]},
)
assert dims == dict(N=3, M=2)

shape = infer_dimensions(
    meta=inst.meta,
    values={
        "a-string-array": [("a", "b"), ("c", "d"), ("e", "f")],
        "a-fixstring-array": [
            ("a", "b", "c"),
            ("a", "b", "c"),
            ("a", "b", "c"),
        ],
    },
)
assert shape == dict(N=3, M=2)

shape = infer_dimensions(
    meta=inst.meta,
    values={
        "an-int-array": [1, 2, 3, 4],
        "a-fixstring-array": [("Al", "Mg"), ("Si", "Cu")],
    },
)
assert shape == dict(N=2, M=4)


# PR #677: test that infer_dimensions() correctly handles ref types
Item = dlite.get_instance("http://onto-ns.com/meta/0.1/Item")
item1 = Item([2], properties={"name": "a", "f": [3.14, 2.72]})
item2 = Item([3], properties={
    "name": "b", "f": [float("-inf"), 0, float("inf")]
})
dims = infer_dimensions(
    meta=Item, values=item1.asdict(single=True)["properties"]
)
assert dims == {"nf": 2}

Ref = dlite.get_instance("http://onto-ns.com/meta/0.1/Ref")
ref = Ref(dimensions={"nitems": 2, "nrefs": 1})
ref.item = item1
ref.items = item1, item2
ref.refs = [ref]
dims = infer_dimensions(meta=Ref, values=ref.asdict(single=True)["properties"])
assert dims == {"nitems": 2, "nrefs": 1}


# Test DictStore
ds = DictStore()
ds.add({
    "meta": "http://onto-ns.com/meta/0.1/Collection",
    "dimensions": [0],
    "properties": {"relations": []},
})
