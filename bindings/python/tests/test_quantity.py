#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" Unit Tests on the quantity module """

from pathlib import Path
import numpy as np
import dlite
from dlite.testutils import raises
from dlite.quantity import get_quantity_helper
try:
    import pint
except ImportError:
    exit(44)  # skip test

thisdir = Path(__file__).absolute().parent
entitydir = thisdir / "entities"
dlite.storage_path.append(f'{entitydir}/*.json')

Person = dlite.get_instance("http://onto-ns.com/meta/0.1/PersonQuantity")


def assertQuantity(q, m, u):
    if np.isnan(m):
        assert np.isnan(q.m)
    else:
        assert q.m == m
    assert q.u == pint.Unit(u)


def assertParse(h, value, m, u):
    assertQuantity(h.parse(value), m, u)


# Test the quantity helper methods: names, units, values, items
inst = Person(dimensions={'N': 3})
names = inst.q.names()
assert ['age', 'length', 'temperature', 'stress'] == names
assert ['years', 'cm', 'degC', 'MPa'] == inst.q.units()
inst.q.update(
    temperature='20 degC',
    length=(22.3, 'mm'),
    age='1461 days',
    stress=([1, 2, 3], 'GPa')
)
values = inst.q.values()
assertQuantity(values[0], 4.0, 'year')
assertQuantity(values[1], 2.23, 'cm')
assertQuantity(values[2], 20.0, 'degC')
assert np.allclose(values[3].m, [1000.0, 2000.0, 3000.0])
assert values[3].u == pint.Unit('MPa')
items = list(inst.q.items())
names = [item[0] for item in items]
assert ['age', 'length', 'temperature', 'stress'] == names

# Test the quantity helper for dlite instances
q = inst.q
# raise a ValueError if property does not exists
with raises(ValueError):
    q.unknown
# raise a TypeError if the property has not quantity type
with raises(TypeError):
    inst.q.name
# modify the value of the property "temperature"
inst.temperature = 37.0
# get temperature as quantity
assertQuantity(q.temperature, 37.0, 'degC')
assertQuantity(inst.get_quantity('temperature'), 37.0, 'degC')
# use the quantity to scale the magnitude
assertQuantity(q.temperature.to('K'), 273.15 + 37.0, 'K')

# Test to set quantity into dlite instance
inst.name = 'Hello'
assert inst.name == 'Hello'
with raises(TypeError):
    inst.q.name = 3
inst.q.temperature = '20 degC'
assertQuantity(inst.q.temperature, 20, 'degC')
inst.q['length'] = (10, 'mm')
assertQuantity(inst.q.length, 1, 'cm')
inst.q.age = inst.q.quantity(1461, 'days')
assertQuantity(inst.q.age, 4, 'years')
surface = inst.q.length ** 2
assertQuantity(surface, 1, 'cm²')
force = inst.q.quantity([1, 2, 3], 'kN')
inst.q.stress = force / surface
assert np.allclose([10, 20, 30], inst.q.stress.m)
assert pint.Unit('MPa') == inst.q.stress.u
inst.set_quantity('age', 365.25, 'days')
assertQuantity(inst.get_quantity('age'), 1.0, 'year')

# test parse expression
p = get_quantity_helper(None)
assertParse(p, '15', 15, '')
assertParse(p, '273.15K', 273.15, 'K')
assertParse(p, 'degC', 1.0, 'degC')
assertParse(p, 'degC/cm', 1.0, 'degC/cm')
assertParse(p, '', np.nan, '')
assertParse(p, None, np.nan, '')
assertParse(p, 'None', np.nan, '')
assertParse(p, 'none', np.nan, '')
assertParse(p, 'Text', np.nan, '')
assertParse(p, 'degC/28', 1 / 28, 'degC')
assertParse(p, '3.0', 3.0, '')
assertParse(p, '10cm', 10, 'cm')
assertParse(p, (10.0, 'km'), 10, 'km')
assertParse(p, 10.0, 10.0, '')
arr = [1.2, 2.3, 3.4]
q = p.quantity(arr, 'nm')
assert np.allclose(arr, q.m)
assert q.u == pint.Unit('nm')

# test unit "percent"
assertParse(p, '1.1', 1.1, '')
assertParse(p, '11%', 11, 'percent')
q = p.parse('11%').to('')
assert q.m == 0.11
q = p.parse('11%').to('percent')
assert q.m == 11
q = p.parse('101 percent').to('')
assert q.m == 1.01
q = p.parse('101 1/%').to('')
assert q.m == 10100.0
