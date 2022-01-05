#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

from dlite import Instance, Collection

thisdir = os.path.abspath(os.path.dirname(__file__))


coll = Collection()

coll.add_relation('cat', 'is-a', 'animal')
coll.add_relation('dog', 'is-a', 'animal')

itr = coll.get_iter()
while itr.poll():
    print(itr.find())

url = 'json://' + thisdir + '/MyEntity.json' + "?mode=r"

# Load metadata (i.e. an instance of meta-metadata) from url
e = Instance.create_from_url(url)

inst1 = Instance.create_from_metaid(e.uri, [3, 2])
inst2 = Instance.create_from_metaid(e.uri, (3, 4), 'myinst')

coll.add('inst1', inst1)
coll.add('inst2', inst2)

assert coll.count() == 2
print(list(coll))


del inst1
del inst2
del coll
