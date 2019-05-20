#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys

import numpy as np

import dlite
from dlite import Storage, Instance

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json' #+ "?mode=r"



# Load metadata (i.e. an instance of meta-metadata) from url
s = Storage(url)
myentity = Instance(s, 'http://meta.sintef.no/0.1/MyEntity')
del s

with Storage(url) as s2:
    myentity2 = Instance(s2, 'http://meta.sintef.no/0.1/MyEntity')
