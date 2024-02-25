Simple demos showing how to work with mappings
==============================================
This example demonstrates how a user can instantiate a certain data
model given existing data that is available as instances of other data
models.

The example is provided at three levels:

1. **simple.py**: Run a simplified example without mapping functions.
                           
Note that this does not return an instance of the **Molecule** (see description of example below),  
but rather an instance of **CalcResult**, which contains both information about the potential energy
and forces. Also, this example does involve any molecular structure as starting point. 
The **CalcResult** instance is populated through mappings from a **Forces** instance and an **Energy** instance.

2. **mappingfunc.py** Utilise mapping functions as illustrated in the figure below.

   Install extra dependencies with `pip install tripper`

3. **oteexample.py** Utilise mapping functions as illustrated in the figure below
   using OTEAPI.

   Install extra dependencies with `pip install -r requirements.txt`



Example case
------------
**Molecule** is the wanted data model.  It is instantiated by:

1. Loading a given atom structure from a data source into an instance
   of a **Structure** data model. Note that this is a datmodel describing an
   atomistic structure with symbols and positions of the atoms as well as the unit cell vectors.

2. Calculating the potential energy and the forces on all atoms using
   the EMT calculator and storing them in an instance of a
   **CalcResults** data model.

3. Mapping properties of the data models and input/output of mapping
   functions to ontological concepts.

4. Invoking `dlite.mappings.instantiate()` to instantiate a
   **Molecule** data model from the mappings.

The process is shown in the figure below.  Red arrows indicate mappings.
![asedemo](asedemo.svg)
