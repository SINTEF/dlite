#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" Unit Tests on the quantity module """

from pathlib import Path
import unittest
import numpy as np
import dlite
from dlite.quantity import get_quantity_helper
try:
    import pint
except ImportError:
    exit(44)  # skip test

thisdir = Path(__file__).absolute().parent
entitydir = thisdir / "entities"
dlite.storage_path.append(f'{entitydir}/*.json')

Person = dlite.get_instance("http://onto-ns.com/meta/0.1/PersonQuantity")


class QuantityUnitTest(unittest.TestCase):

    def assertQuantity(self, q, m, u):
        if np.isnan(m):
            self.assertTrue(np.isnan(q.m))
        else:
            self.assertEqual(q.m, m)
        self.assertEqual(q.u, pint.Unit(u))

    def assertParse(self, h, value, m, u):
        self.assertQuantity(h.parse(value), m, u)

    def test_items(self):
        """ Test the quantity helper methods: names, units, values, items """
        inst = Person(dimensions={'N': 3})
        names = inst.q.names()
        self.assertEqual(['age', 'length', 'temperature', 'stress'], names)
        self.assertEqual(['years', 'cm', 'degC', 'MPa'], inst.q.units())
        inst.q.update(
            temperature='20 degC',
            length=(22.3, 'mm'),
            age='1461 days',
            stress=([1, 2, 3], 'GPa')
        )
        values = inst.q.values()
        self.assertQuantity(values[0], 4.0, 'year')
        self.assertQuantity(values[1], 2.23, 'cm')
        self.assertQuantity(values[2], 20.0, 'degC')
        self.assertTrue(np.allclose(values[3].m, [1000.0, 2000.0, 3000.0]))
        self.assertEqual(values[3].u, pint.Unit('MPa'))
        items = list(inst.q.items())
        names = [item[0] for item in items]
        self.assertEqual(['age', 'length', 'temperature', 'stress'], names)

    def test_get(self):
        """ Test the quantity helper for dlite instances """
        inst = Person(dimensions={'N': 3})
        q = inst.q
        # raise a ValueError if property does not exists
        with self.assertRaises(ValueError):
            q.unknown
        # raise a TypeError if the property has not quantity type
        with self.assertRaises(TypeError):
            inst.q.name
        # modify the value of the property "temperature"
        inst.temperature = 37.0
        # get temperature as quantity
        self.assertQuantity(q.temperature, 37.0, 'degC')
        self.assertQuantity(inst.get_quantity('temperature'), 37.0, 'degC')
        # use the quantity to scale the magnitude
        self.assertQuantity(q.temperature.to('K'), 273.15 + 37.0, 'K')

    def test_set(self):
        """ Test to set quantity into dlite instance """
        inst = Person(dimensions={'N': 3})
        inst.name = 'Hello'
        self.assertEqual(inst.name, 'Hello')
        with self.assertRaises(TypeError):
            inst.q.name = 3
        inst.q.temperature = '20 degC'
        self.assertQuantity(inst.q.temperature, 20, 'degC')
        inst.q['length'] = (10, 'mm')
        self.assertQuantity(inst.q.length, 1, 'cm')
        inst.q.age = inst.q.quantity(1461, 'days')
        self.assertQuantity(inst.q.age, 4, 'years')
        surface = inst.q.length ** 2
        self.assertQuantity(surface, 1, 'cm²')
        force = inst.q.quantity([1, 2, 3], 'kN')
        inst.q.stress = force / surface
        self.assertTrue(np.allclose([10, 20, 30], inst.q.stress.m))
        self.assertEqual(pint.Unit('MPa'), inst.q.stress.u)
        inst.set_quantity('age', 365.25, 'days')
        self.assertQuantity(inst.get_quantity('age'), 1.0, 'year')

    def test_parse_expression(self):
        p = get_quantity_helper(None)
        self.assertParse(p, '15', 15, '')
        self.assertParse(p, '273.15K', 273.15, 'K')
        self.assertParse(p, 'degC', 1.0, 'degC')
        self.assertParse(p, 'degC/cm', 1.0, 'degC/cm')
        self.assertParse(p, '', np.nan, '')
        self.assertParse(p, None, np.nan, '')
        self.assertParse(p, 'None', np.nan, '')
        self.assertParse(p, 'none', np.nan, '')
        self.assertParse(p, 'Text', np.nan, '')
        self.assertParse(p, 'degC/28', 1 / 28, 'degC')
        self.assertParse(p, '3.0', 3.0, '')
        self.assertParse(p, '10cm', 10, 'cm')
        self.assertParse(p, (10.0, 'km'), 10, 'km')
        self.assertParse(p, 10.0, 10.0, '')
        arr = [1.2, 2.3, 3.4]
        q = p.quantity(arr, 'nm')
        self.assertTrue(np.allclose(arr, q.m))
        self.assertEqual(q.u, pint.Unit('nm'))

    def test_unit_percent(self):
        p = get_quantity_helper(None)
        self.assertParse(p, '1.1', 1.1, '')
        self.assertParse(p, '11%', 11, 'percent')
        q = p.parse('11%').to('')
        self.assertEqual(q.m, 0.11)
        q = p.parse('11%').to('percent')
        self.assertEqual(q.m, 11)
        q = p.parse('101 percent').to('')
        self.assertEqual(q.m, 1.01)
        q = p.parse('101 1/%').to('')
        self.assertEqual(q.m, 10100.0)


if __name__ == '__main__':
    unittest.main()
