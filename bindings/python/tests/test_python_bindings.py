#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import print_function

import os
import sys
import subprocess
import unittest

thisdir = os.path.dirname(__file__)


def runfile(filename):
    return subprocess.call([sys.executable, os.path.join(thisdir, filename)])


# Wrap tests into a unittest TestCase
# This way, we can run the individual tests interactively
class TestDLite(unittest.TestCase):
    def test_misc(self):
        self.assertEqual(runfile('test_misc.py'), 0)

    def test_entity(self):
        self.assertEqual(runfile('test_entity.py'), 0)

    def test_entity(self):
        self.assertEqual(runfile('test_storage.py'), 0)


if __name__ == "__main__":
    unittest.main()
