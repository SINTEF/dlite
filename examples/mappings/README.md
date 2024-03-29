Simple demos showing how to work with mappings
==============================================
This example demonstrates how a user can instantiate a certain data
model given existing data that is available as instances of other data
models.

The example is provided at three levels:

1. **simple.py**: Run a simplified example without mapping functions.

   Note that this does not return an instance of the **Molecule** (see
   description of example below), but rather an instance of **CalcResult**,
   which contains both information about the potential energy and forces.

   Also, this example does involve any molecular structure as starting point.
   The **CalcResult** instance is populated through mappings from a
   **Forces** instance and an **Energy** instance.

2. **mappingfunc.py** Utilise mapping functions as illustrated in the figure below.

   Install extra dependencies with `pip install tripper`

3. **oteexample.py** Utilise mapping functions as illustrated in the figure below
   using OTEAPI.


Example case
------------
**Molecule** is the wanted data model.  It is instantiated by:

1. Loading a given atom structure from a data source into an instance
   of a **Structure** data model. Note that this is a datamodel
   describing an atomistic structure with symbols and positions of the
   atoms as well as the unit cell vectors.

2. Load potential energy and the forces as an instance of a
   **CalcResult** data model.

   The **CalcResult** datamodel contains the calculated energies and forces
   from a force field calculation of a **Structure** instance.

3. Mapping properties of the data models and input/output of mapping
   functions to ontological concepts.

4. Invoking `dlite.mappings.instantiate()` to instantiate a
   **Molecule** data model from the mappings.

The process is shown in the figure below.  Red arrows indicate mappings.
![mappingdemo](mappingdemo.svg)


Running the examples
--------------------
Install dependencies with

    pip install -r requirements.txt

and run the three examples as Python scripts, e.g. `python simple.py`.

`python main.py` will run all three examples.


### Run the example with DLite built from source
Please note that the `requirements.txt` file will install DLite from
PyPI (as DLite-Python).  If you want do run this example with DLite
build from source, you can do the following:

    # Setup & install
    mkdir <BUILD/DIRECTORY>
    cmake [OPTIONS] -B <BUILD/DIRECTORY>
    cd examples/mappings
    pip install -r requirements.txt
    pip uninstall DLite-Python
    cmake --build <BUILD/DIRECTORY>
    cmake --install <BUILD/DIRECTORY>

    # Run tests
    python simple.py
    python mappingfunc.py
    python oteexample.py
