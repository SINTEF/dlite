Pint unit registry generator
============================

Introduction and usage
----------------------

The units.py file contains the get_pint_registry() function, which downloads 
the [QUDT UNITS](https://www.qudt.org/doc/DOC_VOCAB-UNITS.html) vocabulary 
and uses its contents to generate a unit registry file for 
[Pint](https://pint.readthedocs.io). The function then uses the generated 
registry file to generate and return a Pint UnitRegistry object.

The unit registry file is cached, and the default behavior is to not 
re-recreate it if it already exists in the cache directory.

Any unit identifiers in QUDT UNITS that use the "-" or " " characters will 
have these replaced with "_" (in order to be Pint compatible).

The usage of the get_pint_registry() is demonstrated in test_units.py.


Technical details
-----------------
* Unit registry filename: `pint_unit_registry.txt`
* Cache directory in Unix/Linux: typically `~/.cache/dlite`
* Cache directory in Windows 10: typically `C:\Users\<USER>\AppData\Local\
Packages\PythonSoftwareFoundation.Python.<VERSION>\LocalCache\Local\dlite\
dlite\Cache`
* Cache directory in Mac OS X: presumably `~/Library/Caches/dlite` (not tested)

For the units, all identifiers, alternative labels and definitions are 
read from QUDT. The program resolves naming conflicts by omitting the 
conflicting labels, following a certain prioritization. Highest priority is 
given to the rdfs:label identifiers, which are used as the primary 
identifier ("canonical name") in the Pint unit registry.

Prefix definitions are hard-coded and not read from QUDT. Units in QUDT UNITS 
that start with a prefix are omitted, since Pint performs reasoning based on 
the prefix definitions in its unit registry. The "KiloGM" SI unit is 
excepted from this rule.


Known problems
--------------
* The program does not yet work on Windows, see 
[issue #497](https://github.com/SINTEF/dlite/issues/497).

* The program provides warnings (at registry creation time) for **omitted 
units and/or labels**, with details about any label conflicts. This output 
can be used to identify duplications and inconsistencies within QUDT UNITS.

* Since the QUDT UNITS vocabulary is subject to change, so are the omitted 
units and labels. When a unit is assigned a new label in QUDT UNITS, this 
may take precedence over currently existing labels on other units (depending 
on the prioritization of the various label types). Since QUDT UNITS seems 
not to be consistency-checked before release, this can result in (possibly 
undetected) changed references to the units of interest.

* Units that contain a prefix inside its name are currently included in the 
generated Pint unit registry.

