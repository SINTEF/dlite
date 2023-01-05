# Features

- Simple and structured way to represent data as a set of named properties within and between software.
- Simple type system where data type are specified as a basic type and size.
  Supported basic types includes:

  - binary blob (any size)
  - boolean (system-dependent size, typically 8 bits)
  - integer (8, 16, 32, 64 bits)
  - unsigned integer (8, 16, 32, 64 bits)
  - float (32, 64, [80, 128] bits)
  - fixed string (any size, always NUL-terminated)
  - string pointer (pointer to malloc'ed string, NUL-terminated)
  - relation (subject-predicate-object triplet)
  - dimension (only intended for metadata)
  - property (only intended for metadata)

- Supports units and multi-dimensional arrays.
- Fully implemented metadata model as presented by Thomas Hagelien.
- Builtin JSON, HDF5, RDF, YAML, PostgreSQL, csv, blob storage plugins (JSON is always available, the other depends on external libraries).
- Plugin system for user-provided storage drivers.
- Memory for metadata and instances is reference counted.
- Lookup of metadata and instances at pre-defined locations (initiated from the DLITE_STORAGES environment variable).
- Template-based code generation, including templates for C and Fortran.
- Plugin system for mappings that maps instances of a set of input metadata to an output instance.
- Fortran bindings.
- Python bindings.
- Embedding of Python allowing writing storage and mapping plugins in Python.
- Full provenance in instance evolution; see [](../user_guide/transactions.md).
