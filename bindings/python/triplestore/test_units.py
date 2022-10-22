from units import pint_registry_lines_from_qudt, prepare_cache_file_path, get_pint_registry
from pint import UnitRegistry, Quantity


ureg = get_pint_registry(force_recreate=False)

# Test the registry.
test_quantity1 = 1234 * ureg.m
assert str(test_quantity1) == "1234 m"

test_quantity2 = 2345.6 * ureg.W_PER_K
assert str(test_quantity2) == "2345.6 W_PER_K"

test_quantity3 = test_quantity1 * test_quantity2
#print("".join([str(test_quantity3), " = ",
#               str(test_quantity3.to_base_units()),
#               " = ",
#               "{:~}".format(test_quantity3.to_base_units())]))
