#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys
import pickle

import numpy as np

import dlite
from dlite import Instance, Dimension, Property, Relation

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json' #+ "?mode=r"


# Load metadata (i.e. an instance of meta-metadata) from url
myentity = Instance(url)

# Check some properties of the entity
assert myentity.uuid == 'ea34bc5e-de88-544d-bcba-150b7292873d'
assert myentity.uri == 'http://meta.sintef.no/0.1/MyEntity'
assert np.all(myentity.dimensions == [2, 14])  # ndimensions, nproperties
assert not myentity.is_data
assert myentity.is_meta
assert not myentity.is_metameta

# Store the entity to a new file
myentity.save('json://xxx.json')

# Create an instance of `myentity` with dimensions 2, 3
# For convinience, we give it an unique label "myid" that can be used
# interchangable with its uuid
inst = Instance(myentity.uri, [2, 3], 'myid')
assert np.all(inst.dimensions == [2, 3])
assert inst.is_data
assert not inst.is_meta
assert not inst.is_metameta

# Assign properties
inst['a-blob'] = bytearray(b'0123456789abcdef')
inst['a-blob-array'] = [[b'0123', b'4567'], [b'89ab', b'cdef']]
inst['a-bool'] = False
inst['a-bool-array'] = True, False
inst['an-int'] = 42
inst['an-int-array'] = 1, 2, 3
inst['a-float'] = 42.3
inst['a-float64-array'] = 3.14, 5.0, 42.3
inst['a-fixstring'] = 'something'
inst['a-fixstring-array'] = [['Al', 'X'], ['Mg', 'Si']]
inst['a-string'] = 'Hello!'
inst['a-string-array'] = [['a', 'b', 'c'], ['dd', 'eee', 'ffff']]
inst['a-relation'] = dlite.Relation('dog', 'is_a', 'mammal')
inst['a-relation'] = ['dog', 'is_a', 'mammal']
inst['a-relation'] = dict(s='dog', p='is_a', o='mammal')
inst['a-relation-array'] = [
    ('cheep', 'is_a', 'mammal'),
    #('cat', 'is_a', 'mammal'),
    #dlite.Relation('cheep', 'is_a', 'mammal'),
    dlite.Relation('cat', 'is_a', 'mammal'),
    ]

# Print the value of all properties
for i in range(len(inst)):
    print('prop%d:' % i, inst[i])

# Strin representation (as json)
print(inst)

# Check save and load
inst.save('json://inst.json')
inst2 = Instance('json://inst.json')

# Check pickling
s = pickle.dumps(inst)
inst3 = pickle.loads(s)

dim = Dimension('N')

prop = Property("a", type='float')

# FIXME - property dimensions should be strings!
prop2 = Property("b", type='string10', dims=[2, 3, 4],
                 description='something enlightening...')

props = myentity['properties']
props[0]

assert inst.meta == myentity

e = dlite.get_instance('http://meta.sintef.no/0.1/MyEntity')
assert e == myentity
assert e != inst

e2 = Instance(
    'http://meta.sintef.no/0.1/NewEntity',
    [Dimension('N', 'Number of something')],
    [Property('name', type='string', description='Name of something.'),
     Property('arr', type='int', dims=[0], description='An array.'),
     Property('v', type='double', unit='m/s', description='Velocity')],
    'Something new...')

e3 = Instance(
    'http://meta.sintef.no/0.1/NewEntity2',
    [],
    [Property('name', type='string', description='Name of something.'),
     Property('arr', type='int', description='An array.'),
     Property('v', type='double', unit='m/s', description='Velocity')],
    'Something new...')
