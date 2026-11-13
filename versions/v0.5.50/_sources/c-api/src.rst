
DLite C-API
===========


DLite is a C implementation of the SINTEF Open Framework and Tools
(SOFT), which is a set of concepts and tools for how to efficiently
describe and work with scientific data.

All data in DLite is represented by an Instance, which is build on a
simple data model.  An Instance is identified by a unique UUID and
have a set of named dimensions and properties.  It is described by its
Metadata.  In the Metadata, each dimension is given a name and
description (optional) and each property is given a name, type, shape
(optional), unit (optional) and description (optional).  The shape of
a property refers to the named dimensions.

When an Instance is instantiated, you must suply a value to the named
dimensions.  The shape of the properties will be set according to
that.  This ensures that the shape of the properties are internally
consistent.

A Metadata is also an Instance, and hence described by its
meta-metadata.  By default, DLite defines four levels of metadata;
instance, metadata, metadata schema and basic metadata schema. The
basic metadata schema describes itself, so no further meta levels are
needed.  The idea is if two different systems describes their data
model in terms of the basic metadata schema, they can easily be made
semantically interoperable.

DLite is licensed under the MIT license.


.. toctree::
   :caption: Modules:
   :titlesonly:
   :maxdepth: 1
   :hidden:
   
   src/dlite-arrays
   src/dlite-behavior
   src/dlite-bson
   src/dlite-codegen
   src/dlite-collection
   src/dlite-datamodel
   src/dlite-entity
   src/dlite-errors
   src/dlite-getlicense
   src/dlite-json
   src/dlite-macros
   src/dlite-mapping-plugins
   src/dlite-mapping
   src/dlite-misc
   src/dlite-schemas
   src/dlite-storage-plugins
   src/dlite-storage
   src/dlite-store
   src/dlite-type-cast
   src/dlite-type
   src/dlite
   src/pathshash
   src/triple
   src/triplestore
   src/pyembed
   src/utils
.. Generated with with `respirator` 