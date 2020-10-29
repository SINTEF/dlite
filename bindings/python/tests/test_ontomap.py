#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import requests

from emmo import get_ontology
from dlite.ontomap import Mapping, Metadata, OntoMap


thisdir = os.path.abspath(os.path.dirname(__file__))
inferred = os.path.join(thisdir, 'inferred.owl')
if not os.path.exists(inferred):
    print('Downloading EMMO inferred...')
    url = 'https://emmo-repo.github.io/versions/1.0.0-alpha2/emmo-inferred.owl'
    r = requests.get(url, allow_redirects=True)
    print(len(r.content))
    print(inferred)
    with open(inferred, 'wb') as f:
        f.write(r.content)



#emmo = get_ontology()
emmo = get_ontology(inferred)
emmo.load()


mapping = Mapping(
    {
        'is_a': {
            'target': 'coll',
        },
        'restriction': {
            #'hasProperty': {
            #    '*': {
            #        'target': 'property',
            #        'property': {
            #
            #            },
            #    },
            #},
            'hasPart': {
                '*': {
                    'target': 'coll',
                },
            },
        },
        'class': {
            '*': {
                'dlite_version': '0.1',
                'dlite_namespace': 'http://meta.sintef.no',
            },
        },
        'labelname': 'prefLabel',
    }
)

om = OntoMap(emmo, mapping)
#om.add_metadata('AtomModelEntity')
#om.add_metadata('Ampere')
om.add_metadata('Atom')

coll = om.get_collection()
coll.save('json://xxx.json?mode=w')

print('=' * 79)
print(coll)
