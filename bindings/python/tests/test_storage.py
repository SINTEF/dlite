#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys

import numpy as np

import dlite
from dlite import Storage, Instance

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json' #+ "?mode=r"


s = Storage(url)

# Load metadata (i.e. an instance of meta-metadata) from url
myentity = Instance(s, 'http://meta.sintef.no/0.1/MyEntity')
