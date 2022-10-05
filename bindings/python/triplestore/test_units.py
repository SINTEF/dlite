from units import pint_registry_lines_from_qudt, prepare_cache_file_path, get_pint_registry
from pint import UnitRegistry, Quantity


ureg = get_pint_registry(force_recreate=True)

# Test the registry.
test_quantity1 = 1234 * ureg.M
print(test_quantity1)

test_quantity2 = 2345.6 * ureg.W_PER_K
print(f'{test_quantity2} = {test_quantity2.to_base_units()}')

test_quantity3 = test_quantity1 * test_quantity2
print("".join([str(test_quantity3), " = ", str(test_quantity3.to_base_units()), " = ", "{:~}".format(test_quantity3.to_base_units())]))