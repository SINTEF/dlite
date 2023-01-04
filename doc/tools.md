Tools
=====
DLite comes with a small set of tools.


dlite-getuuid
-------------
This is a handy small tool for generating a random UUID or getting the UUID corresonding to an URI.

Example usage:

```console
# No argument, generate a random UUID
$ dlite-getuuid
b7ffeb70-5ffe-43de-91e0-941244d21d68

# Calculate the UUID corresponding to an URI
$ dlite-getuuid http://onto-ns.com/meta/example/0.1/Example
022b27f0-dd91-516d-83cb-94db8f65e42b

# It recognises if the argument is already a valid UUID
$ dlite-getuuid b7ffeb70-5ffe-43de-91e0-941244d21d68
b7ffeb70-5ffe-43de-91e0-941244d21d68

# ...even when the the UUID is encoded in the form `<META_URI>/<UUID>` optionally followed by a slash (/) or hash (#)
$ dlite-getuuid http://onto-ns.com/meta/0.1/Collection/b7ffeb70-5ffe-43de-91e0-941244d21d68#
b7ffeb70-5ffe-43de-91e0-941244d21d68
```


dlite-codegen
-------------
This is a template-based code generator for C and Fortran (it is not needed for Python due to its dynamic nature).

It comes with four pre-defined templates:

- **c-header**: Generate a C header file for given entity.
  The generated file declares the struct for an instance of the entity it was generated from.
  It can be included and used in your project without any dependencies (except for the header files `boolean.h`, `integers.h` and `floats.h` that are provided with dlite).
- **c-source**: Generates a C source file for a hard-coded instance of an entity.
- **c-meta-header**: Generates a C header file for an entity schema.
- **fortran-module**: Generates a hard-coded instance of an entity.

For example, the following command will generate a C header for the `Person.jon` entity, run

```console
dlite-codegen -f c-header -o person.h Person.json
```

Run `dlite-codegen --help` for more information about options and arguments.


dlite-env
---------
Runs a command with environment variables correctly set up for DLite.

Run `dlite-env --help` for more information about options and arguments.
