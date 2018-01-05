Concepts
========
*dlite* shares the [metadata model of SOFT][1] and generic data stored
with SOFT can be read with dlite and vice verse.  However, apart from
*dlite* being much smaller and less complete, there are also some
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

If the `id` provided to dopen() is not `NULL` or a valid UUID, it is
interpreted as the name of the referred instance.  *dlite* will then
generate a version 5 UUID from the `id` (from the SHA-1 hash of `id`
and the DNS namespace) that the instance will be stored under.

When you later refer to the data by the generated UUID, you can query its
name using dget_dataname().

The `dgetuuid` tool can be used to manually convert instance names to
their corresponding UUID.


Simple unified access to all data types
---------------------------------------
The datamodel API for accessing properties of an instance in SOFT, has
separate getters and setters for each type and number of dimensions.
*dlite* simplifies this.

Firstly, no storage strategy and datamodel has to be created.  Instead
*dlite* provides dget_property() and dset_property(), which directly
interacts with the `DLite` instance returned by dopen().

Secondly, instead of separate functions for each type, dget_property()
and dset_property() takes two arguments, `type` and `size`, uniquely
defining the property type.

Thirdly, instead of separate functions for each number of array
dimensions, dget_property() and dset_property() takes two additional
arguments, `ndims` and `dims` defining the number of dimensions and
size of each dimension.  A bonus is that an arbitrary number of
dimensions is supported.

These three differences, leads to a much smaller and simpler API.


Programming interface
---------------------
See dlite.h for a description of the public API for *dlite*.


[1]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
