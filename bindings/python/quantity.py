
""" Define the singleton QuantityHelper to work with pint Quantity and dlite 
    instance properties.
"""

from typing import Any
import numpy as np
HAS_PINT = True
try:
    import pint
except Exception:
    HAS_PINT = False

DLITE_QUANTITY_TYPES = [
    'int', 'float', 'double', 'uint',
    'int8', 'int16', 'int32', 'int64',
    'uint8', 'uint16', 'uint32', 'uint64',
    'float32', 'float64', 'float80', 'float96', 'float128'
]
quantity_helper = None


class QuantityHelper:

    def __init__(self):
        self.__dict__['_instance'] = None
        self.__dict__['_registry'] = None

    def _get_property(self, name: str):
        """ Return the property if it exists and if it can be a quantity """
        p = self._instance.get_property_descr(name)
        if p.type in DLITE_QUANTITY_TYPES:
            return p
        else:
            raise TypeError(
                f'The type "{p.type}" of property "{name}" is not a type for'
                ' a number, cannot convert the property value to a quantity.'
            )

    def _get_unit_as_string(self, unit: Any) -> str:
        """ Returns the given unit as a string """
        unit_string = ''
        if isinstance(unit, str):
            unit_string = unit.strip()
        elif isinstance(unit, self._registry.Unit):
            unit_string = str(unit)
        return unit_string.replace('%', 'percent')

    def __getitem__(self, name: str) -> pint.Quantity:
        p = self._get_property(name)
        u = self._get_unit_as_string(p.unit)
        return self.quantity(self._instance[name], u)

    def __setitem__(self, name: str, value: Any):
        p = self._get_property(name)
        q = self.parse(value)
        self._instance[name] = q.m if q.units.dimensionless else q.to(p.unit).m

    def __getattr__(self, name: str):
        if name in self.__dict__:
            return self.__dict__[name]
        else:
            return self[name]

    def __setattr__(self, name: str, value: Any):
        if name in self.__dict__:
            self.__dict__[name] = value
        else:
            self[name] = value

    def __call__(self, inst) -> 'QuantityHelper':
        self._instance = inst
        self._registry = pint.get_application_registry()
        if not hasattr(self._registry.get(), 'percent'):
            self._registry.get().define('percent = 0.01*count')
        return self

    def names(self):
        """ Return the names of each property which has a quantity type """
        prop = self._instance.meta['properties']
        return [p.name for p in prop if p.type in DLITE_QUANTITY_TYPES]

    def units(self):
        """ Return the units of each property which has a quantity type """
        prop = self._instance.meta['properties']
        return [p.unit for p in prop if p.type in DLITE_QUANTITY_TYPES]

    def values(self):
        """ Return the quantity of each property which has a quantity type """
        return [self[name] for name in self.names()]

    def items(self):
        """ Return a zip iterator (name, quantity) """
        names = self.names()
        values = [self[name] for name in names]
        return zip(names, values)

    def get(self, *names):
        """ Return the quantity of each given property name """
        if names:
            if len(names) == 1:
                return self[names[0]]
            else:
                return [self[name] for name in names]
        return None

    @property
    def unit_registry(self) -> pint.UnitRegistry:
        """ Returns the current pint UnitRegistry object """
        return self._registry.get()

    def quantity(self, magnitude, unit) -> pint.Quantity:
        """ Return a pint.Quantity object """
        return self._registry.Quantity(magnitude, unit)

    def parse(self, value: Any) -> pint.Quantity:
        """ Parse the given value and return a pint.Quantity object """
        if isinstance(value, (pint.Quantity, self._registry.Quantity)):
            return value
        elif isinstance(value, tuple):
            if len(value) >= 2:
                return self.quantity(value[0], value[1])
            else:
                return self.quantity(value[0], '')
        elif isinstance(value, str):
            return self.parse_expression(value)
        elif value is None:
            return self.quantity(np.nan, '')
        else:
            return self.quantity(value, '')

    def parse_expression(self, value: str) -> pint.Quantity:
        """ Parse an expression (str) and return a pint.Quantity object """
        result = None
        if value:
            value_str = self._get_unit_as_string(value)
            ph = pint.util.ParserHelper.from_string(value_str)
            if len(ph._d) > 0:
                un = '*'.join([f'({k}**{v})' for k, v in ph._d.items()])
                try:
                    unit = self.unit_registry.parse_units(un)
                except Exception:
                    unit = None
                if unit is not None:
                    result = self.quantity(ph.scale, un)
            else:
                result = self.quantity(ph.scale, '')
        return self.quantity(np.nan, '') if result is None else result

    def update(self, **kwargs):
        for key, val in kwargs.items():
            self[key] = val

    def to_dict(self, names=None, value_type='quantity', fmt=''):
        if not isinstance(names, (list, tuple, dict)):
            prop = self._instance.meta['properties']
            names = [p.name for p in prop if p.type in DLITE_QUANTITY_TYPES]
        values = {}
        for name in names:
            q = self[name]
            if value_type.startswith('q'):
                values[name] = q
            elif value_type.startswith('m'):
                values[name] = q.m
            elif value_type.startswith('s'):
                if fmt:
                    q.default_format = fmt
                values[name] = f'{q}'
        return values


def get_quantity_helper(instance):
    global quantity_helper
    if HAS_PINT:
        if quantity_helper is None:
            quantity_helper = QuantityHelper()
        return quantity_helper(instance)
    else:
        raise RuntimeError(
            'you must install "pint" to work with quantities, '
            'try: pip install pint'
        )
