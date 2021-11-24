#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite

assert len(dlite.istore_get_uuids()) == 3 + 4

coll=dlite.Collection()
assert len(dlite.istore_get_uuids()) == 3 + 5


