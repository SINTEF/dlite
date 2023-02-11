Type system
===========

Simple unified access to all data types
---------------------------------------
The datamodel API for accessing properties of an instance in SOFT5 has separate getters and setters for each type and number of dimensions.
DLite generalizes and simplifies this by describing types and dimensionality of properties with 4 parameters:

  - `type`: an enum defining the type of the data item (or items if it has dimensions).
    The table below summarises the implemented data types.
  - `size`: the size of a data item in bytes.
  - `ndims`: number of dimensions.  Scalars has ``ndims=0``.
  - `dims`: array of length `ndims` with the length of each dimension.
    Note: in the future we will rename `dims` to `shape`.  This change has already been implemented in the json parser/serialiser.

By taking these parameters as arguments, the functions `dlite_datamodel_get_property()` and `dlite_datamodel_set_property()` can handle all supported property types.
No storage strategy is needed.

The table below summarises the different data types defined in DLite.
More details, can be found in `src/dlite-type.h`.
Also note that this supports arbitrary dimensional arrays.
All arrays are assumed to be continuous in memory in C-order.
DLite has currently no api for working with arrays as pointers to pointers.

type      | datatype       | sizes                  | description                      | examples
----      | --------       | -----                  | -----------                      | --------
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

The examples shown in curly brackets may not be supported on all platforms.
The size int, uint, float and double are platform-dependent.
For portable applications you should provide the number of bits, like int32, uint32, float32, float64, etc...
Note that the size specification of *blob* and *fixstring* are in bytes (not bits) and that the terminating NUL-character __is__ included in the specified size of the *fixstring* types.
