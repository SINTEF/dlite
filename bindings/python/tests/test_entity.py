#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys

import dlite
from dlite import Instance

thisdir = os.path.abspath(os.path.dirname(__file__))

url = 'json://' + thisdir + '/MyEntity.json' #+ "?mode=r"



myentity = Instance(url)
assert myentity.uuid == 'ea34bc5e-de88-544d-bcba-150b7292873d'
assert myentity.uri == 'http://meta.sintef.no/0.1/MyEntity'

myentity.save_url('json://xxx.json')

inst = Instance(myentity.uri, [2, 3], 'myid')

inst.save_url('json://inst.json')
