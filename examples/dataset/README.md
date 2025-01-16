Representing DLite datamodels as EMMO Datasets
==============================================
The intention with this example is to show how to use the
`dlite.dataset` module to serialise and deserialise DLite datamodels
and instances to and from an EMMO-based RDF representation.

![EMMO-based representation of a datamodel.](https://raw.githubusercontent.com/SINTEF/dlite/652-serialise-data-models-to-tbox/doc/_static/dataset-v2.svg)

The figure above shown how the following simple [`FluidData`]
datamodel is represented with EMMO.

```yaml
uri: http://onto-ns.org/meta/dlite/0.1/FluidData
meta: http://onto-ns.com/meta/0.3/EntitySchema
description: A dataset describing a fluid.
dimensions:
  ntimes: The number of times the measurements are performed.
  npositions: The number of positions the measurements are performed.
properties:
  LJPotential:
    type: string
    description: Reference to Lennart-Jones potential.
  TemperatureField:
    type: float64
    shape: [ntimes, npositions]
    unit: "°C"
    description: Array of measured temperatures.
```

A DLite datamodel is represented with an emmo:Dataset, and its properties are represented with emmo:Datum as illustrated in the figure above.
It shows how a simple [`FluidData`] datamodel is represented.

The datamodel is semantically enhanced using the following mappings
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

Some comments:
- Note the use of `emmo:isDescriptionFor` relations in the mappings. They are stored as-is in the triplestore.

- The `map:mapsTo` are translated to `rdfs:subClassOf` when serialised to the triplestore.


[`FluidData`]: https://github.com/SINTEF/dlite/blob/652-serialise-data-models-to-tbox/examples/dataset/datamodels/FluidData.json
