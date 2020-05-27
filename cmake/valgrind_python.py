# Python script that

import os

import numpy as np

def to_string(byte_sequence):
    return byte_sequence.decode()


s = to_string(b'abc')

A = np.random.rand(2, 2)
