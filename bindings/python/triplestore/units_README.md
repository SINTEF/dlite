Pint unit registry generator
============================

The units.py file contains the get_pint_registry() function, which downloads the QUDT UNITS vocabulary and uses its contents to generate a unit registry file for Pint, https://pint.readthedocs.io.

For the units, all identifiers, alternative labels and definitions are read from QUDT. The program resolves naming conflicts by omitting the conflicting labels, following a certain prioritization. The prefix definitions, on the other hand, are hard-coded and not read from QUDT.

The usage of the get_pint_registry() is demonstrated in test_units.py.

Known problems
--------------

The program provides warnings for omitted units and/or labels, with details about the label conflicts.

