#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json'


# Load metadata (i.e. an instance of meta-metadata) from url
s = dlite.Storage(url)
myentity = dlite.Instance(s, 'http://onto-ns.com/meta/0.1/MyEntity')
del s

with dlite.Storage(url) as s2:
    myentity2 = dlite.Instance(s2, 'http://onto-ns.com/meta/0.1/MyEntity')

# Create an instance
inst = myentity(dims=[2, 3], id='my-data')
inst['a-bool-array'] = True, False


# Test json
print('--- testing json')
myentity.save('json://myentity.json?mode=w')
inst.save('json://inst.json')
del inst
inst = dlite.Instance('json://inst.json#my-data')


# Test yaml
try:
    import yaml
except ImportError:
    pass
else:
    print('--- testing yaml')
    inst.save('yaml://inst.yaml?mode=w')
    del inst
    inst = dlite.Instance('yaml://inst.yaml#my-data')


# Test rdf
try:
    print('--- testing rdf')
    inst.save('rdf:db.xml?mode=w;store=file;filename=inst.ttl;format=turtle')
except dlite.DLiteError:
    print('    skipping rdf test')
else:
    del inst
    # FIXME: read from inst.ttl not db.xml
    inst = dlite.Instance('rdf://db.xml#my-data')
