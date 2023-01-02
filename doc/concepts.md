DLite
=====
DLite is an implementation of [SOFT], which stands for SINTEF Open
Framework and Tools and is a set of concepts for how to achieve
semantic interoperability as well as implementations and corresponding
tooling.

The development of SOFT was motivated by many years of experience with
developing scientific software, where it was observed that a lot of
efforts went into developing parts that had little to do with the
domain.
A significant part of the development process was spent on different
software engineering tasks, such as code design, the handling of I/O,
correct memory handling of the program state and writing import and
export filters in order to use data from different
sources.
In addition comes the code maintenance with support of legacy formats
and the introduction of new features and changes to internal data
state in the scientific software.
With SOFT it is possible to utilize reusable software components that
handle all this, or develop new reusable software components that can
be used by others in the same framework.
At the core of SOFT are the [SOFT data models], which provide a by
design simplistic but powerful way to represent scientific data.

Originally DLite started as a simplified pure C implementation of SOFT
based on [SOFT5], but has with time developed into a robust framework
with a large set of [features].

The main components of DLite is shown in Figure 1, including language
bindings, tools the plugin framework for storages and mappings.

![DLite Architecture](figs/architecture.svg)

_**Figure 1.** DLite Architecture._

DLite contains a core library, implementing a simplistic, but powerful
datamodel-based framework for semantic interoperability.
On top of this, it implements a set of bindings, storages, mappings
and tools.
The library also comes with set of interfaces (API) to create
extensions and custom plugins.

DLite currently provide bindings with well-documented APIs to Python,
C and Fortran.
For C and Fortran it provide tools for code generation for easy and
efficient integration into simulation software.
The Python bindings are dynamic in nature and provide a simple way to
interact with semantic data from Python.

It also provide a plugin architecture for storages and mappings and
comes with a set of generic storages.
The plugins can be written either in C or Python and are available any
of the bindings (including C and Fortran) due to the embedded Python
interpreter.

The main approach to developing software with DLite is to
incrementally describe the domain of the software using data models
(see below).
The data models can represent different elements of the software, and
be used in handling I/O as well as in code generation and
documentation.
Data models can also be used for annotating data and data sets.
This might be useful in cases where for instance the origin of the
data, license and ownership are of importance.

Since any complex software will have many data models and often multiple
instances of the same data model, DLite allows for creating collections of
data models with defined relationships.

One idea of SOFT is that software may be written is such way that
business logic is handled by the codebase, while I/O, file-formats,
version handling, data import/export and interoperability can be
handled by reusable components in the DLite-framework, thus reducing
risk and development time.


SOFT data models
----------------


An entity can be a single thing or object that represents something physical or nonphysical, concretely or abstract. The entity contains information about the data that constitutes the state of thing it describes. The entity does not contain the actual data, but describes what the different data fields are, in terms of name, data types, units, dimensionality etc. Information about data is often called meta data. Formal meta data enables for the correct interpretation of a set of data, which otherwise would be unreadable.

An example of an entity is 'Atom', which can be defined as something that has a position, an atomic number (which characterizes the chemical element), mass, charge, etc. Another example of a completely different kind of entity can be a data reference-entity with properties such as name, description, license, access-url, media-type, format, etc). The first entity is suitable as an object in a simulation code, while the latter is more suitable for a data catalog distribution description (see dcat:Distribution). Entities allows for describing many aspects of the domain. While each entity describes a single unit of information, a collection of entities can describe the complete domain. See collections below.

Uniqueness
Each published entity needs to be uniquely identified in order to avoid confusion. The entity identifier has therefore 3 separate elements: a name, a namespace and a version number. An entity named 'Particle' is unlikely to have the same meaning and the set of parameters across all domains. In particle physics, the entity 'Particle' would constitute matter and radiation, while in other fields the term 'Particle' can be a general term to describe something small. For this reason the SOFT5 entities have namespaces, similar to how vocabularies are defined in OWL. The version number is a pragmatic solution to handle how properties of an Entity might evolve during the development process. In order to handle different versions of a software, the entity version number can be used to identify the necessary transformation between two data sets.






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

DLite also allow to refer to instances using id's of the form
'`namespace`/`version`/`name`/`uuid`' (e.g:
http://onto-ns.com/meta/0.1/Collection/db6e092b-20f9-44c1-831b-bd597c96daae),
where the '`namespace`/`version`/`name`' part is the URI of the
metadata and '`uuid`' is the UUID of the instance.  This has the
advantage that the the URI of an instance will be a valid [RDF]
subject or object in a knowledge base.  In the Python bindings, the
`Instance.get_uri()` method and `Instance.namespace` property will return a
string in this format if the instance has no URI.


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

[SOFT]: https://www.sintef.no/en/publications/publication/1553408/
[SOFT data models]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
[SOFT5]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md
[features]: features.md
[RFC 3986]: https://datatracker.ietf.org/doc/html/rfc3986
[valid URI]: https://en.wikipedia.org/wiki/Uniform_Resource_Identifier#syntax
[RDF]: https://en.wikipedia.org/wiki/Semantic_triple
[fig1]: SOFT-metadata-structure.png "Figure 1. Metadata structure."
