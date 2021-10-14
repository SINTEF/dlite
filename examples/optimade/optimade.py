#!/usr/bin/env python3

import dlite

Structure = dlite.Instance('json://OPTIMADEStructure.json')


struct = Structure((4, 1, 3, 3))

struct.id = 'odbx/2'
struct.type = 'structures'
struct.immutable_id = '7fcb943c-2621-4e22-8713-e6b448665301'
struct.elements = ['Al'] * 4
