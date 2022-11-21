from units import get_pint_registry
from pint import UnitRegistry, Quantity


ureg = get_pint_registry(force_recreate=True)

# Test the registry.
test_quantity1 = 1234 * ureg.m
print(str(test_quantity1.to_base_units()))
assert str(test_quantity1) == "1234 Meter"

test_quantity2 = 2345.6 * ureg.Watt_per_Kelvin
print(str(test_quantity2))
assert str(test_quantity2) == "2345.6 Watt_per_Kelvin"

test_quantity3 = test_quantity1 * test_quantity2
print("".join([str(test_quantity3), " = ",
               str(test_quantity3.to_base_units()),
               " = ",
               "{:~}".format(test_quantity3.to_base_units())]))
