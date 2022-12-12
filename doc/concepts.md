Relation to SOFT
================
SOFT stands for SINTEF Open Framework and Tools and is a set of
concepts for how to achieve semantic interoperability as well as
implementations and tooling.  At the core of SOFT are the [SOFT data
models], which provide a by design simplistic but powerful way to
represent scientific data.

DLite is one implementation of the [SOFT data models].  Originally it
was a simplified pure C implementation of SOFT5, but has with time
developed into a project of its own.  This document will highlight a
few notable differences with SOFT5.


Named data instances
--------------------
In SOFT all instances are referred to by their UUID.  All metadata are
uniquely identified by their URI, which must be of the form
'`namespace`/`version`/`name`', like:
http://onto-ns.com/meta/0.3/EntitySchema.  This is also valid in
DLite, but we allow the URI to optionally end with a hash (#) or slash
(/), which will be ignored.  Since a metadata is an instance of its
meta-metadata, it also has an UUID, which is calculated from the URI
(as an UUID version 5 SHA-1 hash of the URI using the DNS namespace).

In some cases when you have unique and immutable data, e.g. default
input parameters to a given version of a software model, it may be
more convenient to refer to an unique human understandable name (URI),
like "`mymodel-1.2.3-default_input`", rather than a UUID on the form
"`8290318f-258e-54e2-9838-bb187881f996`".  DLite supports this.
Currently DLite does not enforce that user-defined URIs must follow the
[RFC 3986] standard for a [valid URI], but it is recommended to
follow it.

The tool `dlite-getuuid` can be used to manually convert URIs to their
corresponding UUIDs.

A resent feature in DLite is that data instances with no user-defined
URI, will be assigned a default URI of the form
'`namespace`/`version`/`name`/`uuid`' (e.g:
http://onto-ns.com/meta/0.1/Collection/db6e092b-20f9-44c1-831b-bd597c96daae),
where the '`namespace`/`version`/`name`' part is the URI of the
metadata and '`uuid`' is the UUID of the instance.  This has the
advantage that the the URI of an instance will be a valid [RDF]
subject or object in a knowledge base.


Simple unified access to all data types
---------------------------------------
The datamodel API for accessing properties of an instance in SOFT, has
separate getters and setters for each type and number of dimensions.
DLite generalize and simplifies this by describing types and
dimensionality of properties with 4 parameters:

  - `type`: an enum defining the type of the data item (or items if
    it has dimensions), e.g. whether it is an integer, float or string...
    The table below summarises the implemented dtype's.
  - `size`: the size of a data item in bytes.
  - `ndims`: number of dimensions.  Scalars has ``ndims=0``.
  - `dims`: array of length `ndims` with the length of each dimension.

By taking these parameters as arguments, the functions
DLite_datamodel_get_property() and DLite_datamodel_set_property() can handle
all supported property types.  No storage strategy is needed.

The table below summarises the different dtypes defined in DLite.  For
more details, see dlite-type.h.  Also note that this supports arbitrary
dimensional arrays.  All arrays are assumed to be continuous in memory
in C-order.  DLite has currently no api for working with arrays as
pointers to pointers.

type      | dtype          | sizes                  | description                      | examples
----      | -----          | -----                  | -----------                      | --------
blob      | dliteBlob      | any                    | binary blob, sequence of bytes   | blob32, blob128, ...
bool      | dliteBool      | sizeof(bool)           | boolean                          | bool
int       | dliteInt       | 1, 2, 4, {8}           | signed integer                   | (int), int8, int16, int32, {int64}
uint      | dliteUInt      | 1, 2, 4, {8}           | unsigned integer                 | (uint), uint8, uint16, uint32, {uint64}
float     | dliteFloat     | 4, 8, {10, 12, 16}     | floating point                   | (float), (double), float32, float64, {float80, float96, float128}
fixstring | dliteFixString | any                    | fix-sized NUL-terminated string  | string20, string4000, ...
ref       | dliteRef       | sizeof(DLiteInstance*) | reference to another instance    | ref, http://onto-ns.com/meta/0.1/MyEntity
string    | dliteStringPtr | sizeof(char *)         | pointer to NUL-terminated string | string
relation  | dliteRelation  | sizeof(DLiteRelation)  | subject-predicate-object triplet | relation
dimension | dliteDimension | sizeof(DLiteDimension) | only intended for metadata       | dimension
property  | dliteProperty  | sizeof(DLiteProperty)  | only intended for metadata       | property

The examples shown in curly parenthesis may not be supported on all
platforms.  The size int, uint, float and double are
platform-dependent.  For portable applications you should to provide
the number of bits, like int32, uint32, float32, float64, etc...  Note
that the size specification of *blob* and *fixstring* are in bytes
(not bits) and that the terminating NUL-character __is__ included in the
specified size of the *fixstring* types.


Instances, entities, metadata, meta-metadata, etc...
----------------------------------------------------
A metadata structure following the concepts of SOFT and an API to work
with it, is implemented in dlite-entity.h / dlite-entity.c and shown
graphically in Figure 1.

![Metadata structure][fig1]

The actual data or *Data instances* are instances of the *Entity*
describing them.  *Entities* are instances of the *Entity schema*
which describes an *Entity*.  The *Entity schema* is an instance of
the *Basic metadata schema* describing it, which can describe itself
(and can be considered as an instance of itself).  Hence, **everything
is an instance**.  So in contrast to SOFT, *entities* in DLite are just
a special type of *metadata*.

*Collections* are a special type of instances containing references to
a set of set of instances and relationships between them.  They are
currently not yet implemented in DLite.

Instances can be subdivided into:

  - *Data instances* containing actual data.  These are serialised with a
    minimal header, only containing:
      - The UUID identifying the instance.
      - An optional reference to an URI uniquely identifying the instance.
        If given, the UUID is derived from it.
      - A reference (URI) to its metadata.

    This header is then followed by then followed by the content,
    i.e. the size of each dimension and the values of each property.

    A basic `DLiteInstance` type is defined that all data instances
    (including metadata) can be cast into.

    In the figure above, the *Data instances* and the *Collections* are
    both examples of *pure instances*.

  - Metadata.  Entities, Entity schema, Basic metadata schema, etc are
    all examples of metadata.  Metadata is typically identified by an
    URI of the form `namespace/version/name`.  When storing entities
    (as instances) an UUID will be derived from this URI.

    A basic `DLiteMetadata` type is defined, that all metadata can be
    cast into.  Since metadata also are instances, they header starts
    with the same header as DLiteInstance, but includes more fields
    needed to describe their instances.  Entities are a special case
    of metadata, whos instances are the actual data.

    All metadata is immutable.


Metadata semantics
------------------
The semantics used to by any type of metadata to describe its instances
contains three elements:

  - dimensions
  - properties
  - relations

The three first properties of all metadata schemas (metadata who's
instances are metadata) must be "dimensions", "properties" and
"relations" in this order.  However, it is possible to omit
"relations" if the metadata instance has no other properties.


Naming dimensions and properties
--------------------------------
The names of your dimensions and properties should be valid C identifiers,
not starting with underscore.  Another restriction is that should not be
one of the following keywords:

  - uuid
  - uri
  - meta
  - iri

Furthermore, while the following dimension and property names are
actively used in metadata, they must not be used in data instances:

  - ndimensions
  - nproperties
  - nrelations
  - dimensions
  - properties
  - relations

---

[SOFT data models]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
[RFC 3986]: https://datatracker.ietf.org/doc/html/rfc3986
[valid URI]: https://en.wikipedia.org/wiki/Uniform_Resource_Identifier#syntax
[RDF]: https://en.wikipedia.org/wiki/Semantic_triple
[fig1]: SOFT-metadata-structure.png "Figure 1. Metadata structure."
