# Simple python script to run when generating suppression file for python
# More modules should maybe be loaded - whatever you do, don't import dlite!


# Invoke the import machinery...
import os

import numpy as np


# Do some encoding and decofing...
def to_string(byte_sequence):
    return byte_sequence.decode()


s = to_string(b"abc")
b = s.encode()


# Use numpy...
A = np.random.rand(2, 2)
B = np.random.rand(2)
C = np.linalg.solve(A, B)


# Evaluate/execute code
x = eval("2+2")
exec("import sys; sys.modules.keys()")


# create sequences and dicts
t = ("a", 1, np)
s = set(t)
l = list(s)
d = dict(t=t, s=s, l=l)
