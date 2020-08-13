# Simple python script to run when generating suppression file for python
# More modules should maybe be loaded - whatever you do, don't import dlite!

import os

import numpy as np

def to_string(byte_sequence):
    return byte_sequence.decode()


s = to_string(b'abc')

A = np.random.rand(2, 2)
print(A)
