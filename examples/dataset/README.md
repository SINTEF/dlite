Representing DLite datamodels as EMMO Datasets
==============================================
The intention with this example is to show how to use the
`dlite.dataset` module to serialise and deserialise DLite datamodels
and instances to and from an EMMO-based RDF representation.

A DLite datamodel is represented with an emmo:DataSet, and its
properties are represented with emmo:Datum as illustrated in the
figure below.




![EMMO-based representation of a datamodel.](https://raw.githubusercontent.com/SINTEF/dlite/652-serialise-data-models-to-tbox/doc/_static/dataset-v2.svg)


The main interface is exposed by four new functions:
- `add_dataset()`: stores datamodel+mappings to a triplestore
- `add_data()`: stores an instance (or datamodel) to a triplestore
- `get_dataset()`: loads datamodel+mappings from a triplestore
- `get_data()`: loads an instance (or datamodel) from a triplestore



Two tests are added using a datamodel matching what is shown in the figure.
- [test_dataset1_dave.py](https://github.com/SINTEF/dlite/blob/652-serialise-data-models-to-tbox/bindings/python/tests/test_dataset1_save.py) loads first a [`FluidData`]( https://github.com/SINTEF/dlite/blob/652-serialise-data-models-to-tbox/bindings/python/tests/entities/FluidData.json) datamodel and documents it semantically with the following mappings
  ```python
  mappings = [
    (FLUID,                  EMMO.isDescriptionFor, EMMO.Fluid),
    (FLUID.LJPotential,      MAP.mapsTo,            EMMO.String),
    (FLUID.LJPotential,      EMMO.isDescriptionFor, EMMO.MolecularEntity),
    (FLUID.TemperatureField, MAP.mapsTo,            EMMO.ThermodynamicTemperature),
    (FLUID.ntimes,           MAP.mapsTo,            EMMO.Time),
    (FLUID.npositions,       MAP.mapsTo,            EMMO.Position),
  ]
  ```
  Note the use of `emmo:isDescriptionFor` relations in the mappings. They are stored as-is in the triplestore.
  The `map:mapsTo` are translated to `rdfs:subClassOf` when serialised to the triplestore.

  Then it uses the `add_dataset()` function in the new `dlite.dataset` module and stores it as RDF in a local triplestore. The content of the triplestore corresponds now to the figure below.

  Then it creates two `FluidData` instances and store them (using the `add_data()` function) as RDF in a local triplestore as well. The instances are represented as an individual with a rdf:JSON data property containing the instance data.

  Finally the triplestore is serialised to a turtle file.

- [test_dataset2_load.py](https://github.com/SINTEF/dlite/blob/652-serialise-data-models-to-tbox/bindings/python/tests/test_dataset2_load.py) loads the turtle file into a local triplestore and reconstruct the `FluidData` datamodel as well as the mappings using the `get_dataset()` function.

  Using the `get_hash()` method, it is checked that the reconstruct the `FluidData` datamodel is exactly equal to the original datamodel.

  Finally it loads the two instances using the `get_data()` function and check that they are exactly equal to the two original instances.
