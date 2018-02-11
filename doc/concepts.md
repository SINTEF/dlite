Concepts
========
*dlite* shares the [metadata model of SOFT][1] and generic data stored
with SOFT can be read with dlite and vice verse.  However, apart from
*dlite* being much smaller and less complete, there are also a few
notable differences in the API and even conceptual, which are detailed
below.


Named data instances
--------------------
In SOFT all Entity instances are referred to by their UUID.  However,
in some cases when you have unique and immutable data, e.g. default
input parameters to a given version of a software model, it may be
more convenient to refer to a human understandable name, like
"mymodel-1.2.3_default_input", rather than a UUID on the form
"8290318f-258e-54e2-9838-bb187881f996".  *dlite* supports this.

If the `id` provided to dlite_datamodel() or dlite_instance_create()
is not `NULL` or a valid UUID, it is interpreted as an unique uri
referring to the instance.  *dlite* will then generate a version 5
UUID from the `id` (from the SHA-1 hash of `id` and the DNS namespace)
that the instance will be stored under.  The original `id` will also be
stored and can be retrieved with dlite_datamodel_get_meta_uri().

The `dlite-getuuid` tool can be used to manually convert instance names to
their corresponding UUIDs.


Simple unified access to all data types
---------------------------------------
The datamodel API for accessing properties of an instance in SOFT, has
separate getters and setters for each type and number of dimensions.
*DLite* generalize and simplifies this by describing types and
dimensionality of properties with 4 parameters:

  - `dtype`: an enum defining the type of the data item (or items if
    it has dimensions), e.g. whether it is an integer, float or string...
    The table below summarises the implemented dtype's.
  - `size`: the size of a data item in bytes.
  - `ndims`: number of dimensions.  Scalars has ``ndim=0``.
  - `dims`: array of length `ndim` with the length of each dimension.

By taking these parameters as arguments, the functions
dlite_datamodel_get_property() and dlite_datamodel_set_property() can handle
all supported property types.  No storage strategy is needed.

The table below summarises the different dtypes defined in DLite.  For
more details, see dlite-type.h.  Also note that this supports arbitrary
dimensional arrays.  All arrays are assumed to be continuous in memory
in C-order.  *DLite* has currently no api for working with arrays as
pointers to pointers.

type      | dtype          | sizes          | description                      | examples
----      | -----          | -----          | -----------                      | --------
blob      | dliteBlob      | any            | binary blob, sequence of bytes   | blob32, blob128
bool      | dliteBool      | sizeof(bool)   | boolean                          | bool
int       | dliteInt       | 1, 2, 4, {8}   | signed integer                   | (int), int8, int16, int32, {int64}
uint      | dliteUInt      | 1, 2, 4, {8}   | unsigned integer                 | (uint), uint8, uint16, uint32, {uint64}
float     | dliteFloat     | 4, 8, {10, 16} | floating point                   | (float), (double), float32, float64, {float80, float128}
fixstring | dliteFixString | any            | fix-sized NUL-terminated string  | string20
string    | dliteStringPtr | sizeof(char *) | pointer to NUL-terminated string | string


Everything is an instanse
-------------------------
An experimental metadata structure following the concepts of SOFT and
API to work with it, is implemented in dlite-entity.h / dlite-entity.c.
A basic `DLiteInstance` type is defined that all data instances or
metadata can be cast into (since metadata are instances of their
meta-metadata, etc...).  Instances are minimal, with a header containing
only:

  - An UUID.
  - An optional reference to an URI uniquely identifying the instance. If
    given, the UUID is derived from it.  For metadata this URI is of the
    form `namespace/version/name`.
  - A reference to its metadata.

This header is then followed by then followed by content, i.e. the
size of each dimension and the value of each property (and possibly a
set of relations).

A basic `DLiteMetadata` type is also defined, that all metadata can be
cast into.  Since metadata also are instances, they header starts with the
same header as DLiteInstance, but includes more fields needed to describe
their instances.  Entities are a special case of metadata, whos instances
are the actual data.



[1]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
