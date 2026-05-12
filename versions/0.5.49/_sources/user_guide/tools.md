Tools
=====
DLite comes with a small set of tools.


dlite-validate
--------------
The dlite-validate tool can be used to check if a specific representation (in a file) is a valid DLite datamodel.

This can be run as follows
```bash
dlite-validate filename.yaml # or json
```

It will then return a list of errors if it is not a valid datamodel.


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
It makes it simple to use DLite for handling I/O in simulation software written in C or Fortran in an easy to maintain way.

It comes with four pre-defined templates:

- **c-header**: Generate a C header file for given entity.
  The generated file declares the struct for an instance of the entity it was generated from.
  It can be included and used in your project without any dependencies (except for the header files `boolean.h`, `integers.h` and `floats.h` that are provided with dlite).
- **c-source**: Generates a C source file for a hard-coded instance of an entity.
- **c-meta-header**: Generates a C header file for an entity schema.
- **fortran-module**: Generates a hard-coded instance of an entity.

For example, the following command will generate a C header for the `Person.json` entity, run

```console
dlite-codegen -f c-header -o person.h Person.json
```

Run `dlite-codegen --help` for more information about options and arguments.

If you use [cmake] as a build system for your simulation software, you can run `dlite-codegen` via the `dlite_codegen` CMake macro provided with DLite.
By including the following in your CMakeLists.txt file:

```cmake
find_package(dlite REQUIRED)

dlite_codegen(
  person.h
  c_header
  ${CMAKE_CURRENT_SOURCE_DIR}/Person.json
)
```

cmake will generate the `person.h` header file in your binary directory based on the `Person.json` file found in your source directory.
See the C or Fortran examples for a complete example.



dlite-env
---------
Runs a command with environment variables correctly set up for DLite.

Run `dlite-env --help` for more information about options and arguments.



[cmake]: https://cmake.org/
