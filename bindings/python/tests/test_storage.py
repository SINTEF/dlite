#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json'


# Load metadata (i.e. an instance of meta-metadata) from url
s = dlite.Storage(url)
myentity = dlite.Instance.create_from_storage(s, 'http://onto-ns.com/meta/0.1/MyEntity')
del s

with dlite.Storage(url) as s2:
    myentity2 = dlite.Instance.create_from_storage(s2, 'http://onto-ns.com/meta/0.1/MyEntity')
