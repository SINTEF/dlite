#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite
from dlite import Instance, Dimension, Property, Relation

assert len(dlite.istore_get_uuids()) == 3 + 3

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json'

# myentity is already defined via test_global_dlite_state, no new instance is added to istore
myentity = Instance(url)
assert myentity.uri == "http://onto-ns.com/meta/0.1/MyEntity"
assert len(dlite.istore_get_uuids()) == 3 + 3

i1 = Instance(myentity.uri, [2, 3], 'myid')
assert i1.uri == "myid"
assert i1.uuid in dlite.istore_get_uuids()
assert len(dlite.istore_get_uuids()) == 3 + 4

