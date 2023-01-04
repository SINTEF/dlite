from units import get_pint_registry


ureg = get_pint_registry(force_recreate=True)

# Test the registry.
test_quantity1 = 1234 * ureg.M
print(str(test_quantity1.to_base_units()))
assert str(test_quantity1) == "1234 m"

test_quantity2 = 2345.6 * ureg.Watt_per_Kelvin
print(str(test_quantity2))
assert str(test_quantity2) == "2345.6 w/K"

test_quantity3 = test_quantity1 * test_quantity2
print("".join([str(test_quantity3), " = ",
               str(test_quantity3.to_base_units()),
               " = ",
               "{:~}".format(test_quantity3.to_base_units())]))

test_quantity4 = 16 * ureg.S
print(f'{str(test_quantity4)} = {test_quantity4.to_base_units()}')
assert str(test_quantity4) == "16 S"

test_quantity5 = 25 * ureg.s
print(f'{str(test_quantity5)} = {test_quantity5.to_base_units()}')
assert str(test_quantity5) == "25 s"

test_quantity6 = 36 * ureg.exaAmpere
print(f'{str(test_quantity6)} = {test_quantity6.to_base_units()}')
assert str(test_quantity6) == "36 EA"