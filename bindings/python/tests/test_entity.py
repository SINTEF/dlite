#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys

import numpy as np

import dlite
from dlite import Instance, Dimension, Property

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json' #+ "?mode=r"


# Load metadata (i.e. an instance of meta-metadata) from url
myentity = Instance(url)

# Check some properties of the entity
assert myentity.uuid == 'ea34bc5e-de88-544d-bcba-150b7292873d'
assert myentity.uri == 'http://meta.sintef.no/0.1/MyEntity'
assert np.all(myentity.dimensions == [2, 12])  # ndimensions, nproperties

# Store the entity to a new file
myentity.save_url('json://xxx.json')

# Create an instance of `myentity` with dimensions 2, 3
# For convinience, we give it an unique label "myid" that can be used
# interchangable with its uuid
inst = Instance(myentity.uri, [2, 3], 'myid')
assert np.all(inst.dimensions == [2, 3])

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

# FIXME - problems with fixstring arrays
#inst['a-fixstring-array'] = [['Al', 'X'], ['Mg', 'Si']]

inst['a-string'] = 'Hello!'
inst['a-string-array'] = [['a', 'b', 'c'], ['dd', 'eee', 'ffff']]

# Print the value of all properties
for i in range(len(inst)):
    print('prop%d:' % i, inst[i])


# Store the instance
inst.save_url('json://inst.json')


dim = Dimension('N')

prop = Property("a", type=dlite.FloatType, size=4)
prop2 = Property("b", type=dlite.FixStringType, size=10, dims=[2, 3, 4],
                 description='something enlightening...')
