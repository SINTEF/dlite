#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json' + "?mode=r"

E = dlite.Instance(url)
E.iri = 'http://emmo.info/emmo/EMMO_Physical'
E.iri = None
E.iri = 'http://emmo.info/emmo/EMMO_Physical'

e = E([3, 4])
e.iri = 'abc'

p = E['properties'][3]
p.iri = 'http://emmo.info/emmo/EMMO_Length'
