Pint unit registry generator
============================

The units.py file contains the get_pint_registry() function, which downloads the QUDT UNITS vocabulary and uses its contents to generate a unit registry file for Pint, https://pint.readthedocs.io. The function then uses the generated registry file to generate and return a Pint UnitRegistry object.

The unit registry file is cached, and the default behavior is to not re-recreate it if it already exists in the cache directory.

For the units, all identifiers, alternative labels and definitions are read from QUDT. The program resolves naming conflicts by omitting the conflicting labels, following a certain prioritization. The prefix definitions, on the other hand, are hard-coded and not read from QUDT.

The usage of the get_pint_registry() is demonstrated in test_units.py.

Known problems
--------------

The program provides warnings (at registry creation time) for omitted units and/or labels, with details about any label conflicts.

Since the QUDT UNITS vocabulary is subject to change, so are the omitted units and labels.