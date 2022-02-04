#!/usr/bin/env python
import os
import json
from pathlib import Path

import dlite
from dlite.utils import instance_from_dict


thisdir = Path(__file__).absolute().parent
with open(thisdir / 'Person.json', 'rt') as f:
    d = json.load(f)

Person = dlite.utils.instance_from_dict(d)

person = Person([2])
person.name = 'Ada'
person.age = 12.5
person.skills = ['skiing', 'jumping']

d1 = person.asdict()
inst1 = instance_from_dict(d1)
assert inst1.uuid == person.uuid
assert inst1.meta.uuid == Person.uuid
assert inst1.name == 'Ada'
assert inst1.age == 12.5
assert all(inst1.skills == ['skiing', 'jumping'])

d2 = Person.asdict()
inst2 = instance_from_dict(d2)
assert inst2.uuid == Person.uuid


dlite.storage_path.append(thisdir / '*.json')
d = {
    "uuid": "7ee0f569-1355-4eed-a2f7-0fc31378d56c",
    "meta": "http://onto-ns.com/meta/0.1/MyEntity",
    "dimensions": {
        "N": 2,
        "M": 3
    },
    "properties": {
        "a-blob": "00112233445566778899aabbccddeeff",
        "a-blob-array": [
            [
                b"abcd",
                b"efgh"
            ],
            [
                b"ijkl",
                b"mnop"
            ]
        ],
        "a-bool": True,
        "a-bool-array": [
            False,
            True
        ],
        "an-int": 42,
        "an-int-array": [
            -1,
            -2,
            -3
        ],
        "a-float": 3.14,
        "a-float64-array": [
            0.0,
            1.6022e-19,
            6.022e23
        ],
        "a-fixstring": "fix",
        "a-fixstring-array": [
            [
                "one",
                "two"
            ],
            [
                "three",
                "four"
            ]
        ],
        "a-string": None,
        "a-string-array": [
            [
                None,
                "a string",
                None
            ],
            [
                None,
                None,
                None
            ]
        ],
        "a-relation": {
            "s": "a-subject",
            "p": "a-predicate",
            "o": "a-object"
        },
        "a-relation-array": [
            {
                "s": "a1",
                "p": "b1",
                "o": "c1"
            },
            {
                "s": "a2",
                "p": "b2",
                "o": "c2"
            }
        ]
    }
}
inst = instance_from_dict(d)
print(inst)
