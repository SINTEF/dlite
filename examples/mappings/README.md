Simple demo showing how to work with mappings
=============================================
Intended as input to a more rigorous demonstration of ExecFlow.


This demo demonstrates how a a user can instantiate a certain data model given existing data that is available as instances of other data models.

**Molecule** is the wanted data model.  It is instantiated by:
1. Loading a given atom structure from a data source into an instance of a **ASEAtoms** data model.
2. Calculating the potential energy and the forces on all atoms using the EMT calculator and storing them in an instance of a **CalcResults** data model.
3. Mapping properties of the data models and input/output of mapping functions to ontological concepts.
4. Invoking `dlite.mappings.instantiate()` to instantiate a **Molecule** data model from the mappings.

The process is shown in the figure below.  Red arrows indicate mappings.
![asedemo](asedemo.svg)
