#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json'


# Load metadata (i.e. an instance of meta-metadata) from url
s = dlite.Storage(url)
myentity = dlite.Instance(s, 'http://meta.sintef.no/0.1/MyEntity')
del s

with dlite.Storage(url) as s2:
    myentity2 = dlite.Instance(s2, 'http://meta.sintef.no/0.1/MyEntity')
