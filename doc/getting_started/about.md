# About DLite

DLite is a C implementation of [SINTEF Open Framework and Tools (SOFT)][SOFT], which is a set of concepts and tools for how to efficiently describe and work with scientific data.

All data in DLite is represented by an _Instance_, which is built on an elementary data model.
An _Instance_ is identified by a universally unique ID (UUID) and has a set of named dimensions and properties.

An _Instance_ is described by its _Metadata_.
In the _Metadata_, each _dimension_ is given a **name** and **description**, and each _property_ is given a **name**, **type**, **shape**, **unit**, and **description**.
The shape of a property refers to the named dimensions.

When an _Instance_ is instantiated, you supply a value to the named dimensions.
The shape of the properties will be set according to that.
This ensures that the shape of the properties are internally consistent.

A _Metadata_ is also an _Instance_, and hence described by its meta-metadata.
By default, DLite defines four levels of metadata; instance, metadata, metadata schema and basic metadata schema.
The basic metadata schema describes itself, so no further meta levels are needed.
The core idea behind these levels is if two different systems describe their data models in terms of the basic metadata schema, they can easily be made semantically interoperable, while using different metadata schemas.

![The datamodel of DLite.](https://raw.githubusercontent.com/SINTEF/dlite/master/doc/_static/datamodel.svg)

An alternative and more flexible way to enable interoperability is to use ontologies.
DLite provides a specialised _Instance_ called _Collection_.
A _Collection_ is a container holding a set of _Instances_ and relations between them.
But it can also relate an _Instance_ or a dimension or property of an _Instance_ to a concept in an ontology.
DLite allows to transparently map an _Instance_ whose _Metadata_ corresponds to a concept in one ontology to an _Instance_ whose _Metadata_ corresponds to a concept in another ontology.
Such mappings can be registered and reused, providing a very powerful system for achieving interoperability.

DLite provides a common and extendable API for loading and storing _Instances_ from and to different storages.
Additional storage plugins can be written in C or Python.

See the [user guide] for more details.

DLite is licensed under the [MIT license].

## Main features

See [](features.md) for a more detailed list.

- Enables semantic interoperability via formalised metadata and data.
- Metadata can be mapped to or generated from ontologies.
- Code generation for integration in existing code bases.
- Plugin API for data storages (json, hdf5, rdf, yaml, postgresql, blob, csv...).
- Plugin API for mapping between metadata.
- Bindings to C, Python and Fortran.

[SOFT]: https://www.sintef.no/en/publications/publication/1553408/
[user guide]: https://sintef.github.io/dlite/user_guide/index.html
[MIT license]: https://sintef.github.io/dlite/license.html
