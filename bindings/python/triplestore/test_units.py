from units import pint_registry_lines_from_qudt, prepare_cache_file_path, get_pint_registry
from pint import UnitRegistry, Quantity

# pint_registry_lines = pint_registry_lines_from_qudt()

# print(f'Number of registry lines = {len(pint_registry_lines)}')

# Print pint registry definition to file.
# registry_file_path = prepare_cache_file_path("pint_unit_registry.txt")
# with open(registry_file_path, "w") as f:
#     for line in pint_registry_lines:
#         f.write(f"{line}\n")

# Populate an empty pint registry.
#ureg = UnitRegistry(None)
#for line in pint_registry_lines:
#    ureg.define(line)

# for i in range(0, 1751):
#     print("Line number: " + str(i))
#     print(pint_registry_lines[i])
#     with open("test_output.txt", "w") as f:
#         for line in pint_registry_lines[0:i]:
#             f.write(f"{line}\n")
#     ureg = UnitRegistry("test_output.txt")

#ureg = UnitRegistry(registry_file_path)
ureg = get_pint_registry(force_recreate=True)

# Test the registry.
test_quantity1 = 1234 * ureg.M
print(test_quantity1)

test_quantity2 = 2345.6 * ureg.W_PER_K
print(f'{test_quantity2} = {test_quantity2.to_base_units()}')

test_quantity3 = test_quantity1 * test_quantity2
print("".join([str(test_quantity3), " = ", str(test_quantity3.to_base_units()), " = ", "{:~}".format(test_quantity3.to_base_units())]))