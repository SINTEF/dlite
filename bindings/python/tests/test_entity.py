#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys

import dlite
from dlite import Instance

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json' #+ "?mode=r"


# Load metadata (i.e. an instance of meta-metadata) from url
myentity = Instance(url)

# Check some properties of the entity
assert myentity.uuid == 'ea34bc5e-de88-544d-bcba-150b7292873d'
assert myentity.uri == 'http://meta.sintef.no/0.1/MyEntity'
#assert myentity.dimensions == 2, 6  # ndimensions, nproperties

# Store the entity to a new file
myentity.save_url('json://xxx.json')

# Create an instance of `myentity` with dimensions 2, 3
# For convinience, we give it an unique label "myid" that can be used
# interchangable with its uuid
inst = Instance(myentity.uri, [2, 3], 'myid')
#assert inst.dimensions == 2, 3

# Store the instance
inst.save_url('json://inst.json')
