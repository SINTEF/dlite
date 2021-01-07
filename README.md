DLite - lightweight data-centric framework for working with scientific data
===========================================================================

![CI tests](https://github.com/sintef/dlite/workflows/CI%20tests/badge.svg)


DLite is a lightweight cross-platform C library, for working with and
sharing scientific data in an interoperable way.  It is strongly
inspired by [SOFT][1], with the aim to be a lightweight replacement in
cases where Windows portability is a showstopper for using SOFT.

DLite shares the [metadata model of SOFT5][2] and is compatible with
SOFT5 in many respects.  However, it has also some notable
differences, mainly with respect to the type system and that it fully
implements the metadata model envisioned in SOFT5.
See [doc/concepts.md](doc/concepts.md) for details.

DLite is licensed under the MIT license.


Example
-------
Lets say that you have the following Python class

```python
class Person:
    def __init__(self, name, age, skills):
        self.name = name
        self.age = age
        self.skills = skills
```

that you want to describe semantically.  We do that by defining the
following metadata (using json) identifying the Python attributes with
dlite properties.  Here we define `name` to be a string, `age` to be a
float and `skills` to be an array of `N` strings, where `N` is a name
of a dimension.  The metadata uniquely identifies itself with the
"name", "version" and "namespace" fields and "meta" refers the the
metadata schema (meta-metadata) that this metadata is described by.
Finally are human description of the metadata itself, its dimensions
and its properties provide in the "description" fields.

```json
{
  "name": "Person",
  "version": "0.1",
  "namespace": "http://meta.sintef.no",
  "meta": "http://meta.sintef.no/0.3/EntitySchema",
  "description": "A person.",
  "dimensions": [
    {
      "name": "N",
      "description": "Number of skills."
    }
  ],
  "properties": [
    {
      "name": "name",
      "type": "string",
      "description": "Full name."
    },
    {
      "name": "age",
      "type": "float",
      "unit": "year",
      "description": "Age of person."
    },
    {
      "name": "skills",
      "type": "string",
      "dims": ["N"],
      "description": "List of skills."
    }
  ]
}
```

We save the metadata in file "Person.json".  Back in Python we can now
make a dlite-aware subclass of `Person`, instantiate it and serialise
it to a storage:

```python
import dlite

# Create a dlite-aware subclass of Person
DLitePerson = dlite.classfactory(Person, url='json://Person.json')

# Instantiate
person = DLitePerson('Sherlock Holmes', 34., ['observing', 'chemistry',
    'violin', 'boxing'])

# Write to storage (here a json file)
person.dlite_inst.save('json://homes.json?mode=w')
```

To access this new instance from C, you can first generate a header
file from the meta data

```console
$ dlite-codegen -f c-header -o person.h Person.json
```

and then include it in your C program:

```c
// homes.c -- sample program that loads instance from homes.json and prints it
#include <stdio.h>
#include <dlite>
#include "person.h"  // header generated with dlite-codegen

int main()
{
  /* URL of instance to load using the json driver.  The storage is
     here the file 'homes.json' and the instance we want to load in
     this file is identified with the UUID following the hash (#)
     sign. */
  char *url = "json://homes.json#7ac977ce-a0dc-4e19-a7e1-7781c0cd23d2";

  Person *person = dlite_instance_load_url(url);

  int i;
  printf("name:  %s\n", person->name);
  printf("age:   %g\n", person->age);
  printf("skills:\n");
  for (i=0; i<person->N; i++)
    printf("  - %s\n", person->skills[i]);

  return 0;
}
```

Since we are using `dlite_instance_load_url()` to load the instance,
you must link to dlite when compiling this program.  Assuming you are
using Linux and dlite in installed in `$HOME/.local`, compiling with
gcc would look like:

```console
$ gcc -I$HOME/.local/include/dlite -L$HOME/.local/lib -ldlite -o homes homes.c
```

Finally you can run the program with

```console
$ DLITE_STORAGES=*.json ./homes
name:  Sherlock Holmes
age:   34
skills:
  - observing
  - chemistry
  - violin
  - boxing
```

Note that we in this case have to define the environment variable
`DLITE_STORAGES` in order to let dlite find the metadata we stored in
'Person.json'.  There are ways to avoid this, e.g. by hardcoding the
metadata in C using `dlite-codegen -f c-source` or in the C program
explicitely load 'Person.json' before 'homes.json'.

This was just a brief example.  There is much more to dlite.  Since
the documentation is still not complete, the best source is the code
itself, including the tests and examples.


Main features
-------------
See [doc/features.md](doc/features.md) for a more detailed list.
  - Enables semantic interoperability via simple formalised metadata and data
  - Metadta can be linked to or generated from ontologies
  - Code generation for simple integration in existing code bases
  - Plugin API for data storages
  - Plugin API for mapping between metadata
  - Bindings to C, Python and Fortran


Short vocabulary
----------------
The following terms have a special meaning in dlite:
  - **Basic metadata schema**: Toplevel meta-metadata which describes itself.
  - **Collection**: A specialised instance that contains references to set
    of instances and relations between them.  Within a collection instances
    are labeled.  See also the [SOFT5 nomenclauture][SOFT5_nomenclauture].
  - **Data instance**: A "leaf" instance that is not metadata.
  - **Entity**: May be any kind of instance, including data instances,
    metadata instances or meta-metadata instances.  However, for historical
    reasons it is often used for "standard" metadata that are instances of
    meta-metadata "http://meta.sintef.no/0.3/EntitySchema".
  - **Instance**: The basic data object in DLite.  All instances are described
    by their metadata which itself are instances.  Instances are identified
    by an UUID.
  - **iri**: An [internationalized resource identifier (IRI)][IRI] is the
    extension of URI to international characters.  In dlite, the term "iri"
    is used as a reference to a concept in an ontology providing a semantic
    definition of an instance or property.
  - **Mapping**: A function that maps one or more input instances to an
    output instance.  They are an important mechanism for interoperability.
    Mappings are called translators in SOFT5.
  - **Metadata**: a special type of instances that describe other instances.
    All metadata are immutable and has an unique URI in addition to their
    UUID.
  - **Meta-metadata**: metadata that describes metadata.
  - **Relation**: A subject-predicate-object triplet with an id. Relations
    are immutable.
  - **Storage**: A generic handle encapsulating actual storage backends.
  - **Transaction**: A not yet implemented feature, that enables to
    represent the evolution of the state of a software as a series of
    immutable instances.  See also the
    [SOFT5 nomenclauture][SOFT5_nomenclauture].
  - **uri**: A [uniform resource identifier (URI)][URI] is a
    generalisation of URL, but follows the same syntax rules.  In
    dlite, the term "uri" is used as an human readable identifier for
    instances (optional for data instances) and has the form
    `namespace/version/name`.
  - **url**: A [uniform resource locator (URL)][URL] is an reference
    to a web resource, like a file (on a given computer), database
    entry, web page, etc.  In dlite url's refer to a storage or even
    an specific instance in a storage using the general syntax
    `driver://location?options#fragment`, where `options` and `fragment`
    are optional.  If `fragment` is provided, it should be the uuid or
    uri of an instance.
  - **uuid**: A [universal unique identifier (UUID)][UUID] is commonly
    used to uniquely identify digital information.  DLite uses the 36
    character string representation of uuid's to uniquely identify
    instances.  The uuid is generated from the uri for instances that
    has an uri, otherwise it is randomly generated.


Download and build
==================

Download
--------
### From source
DLite sources can be cloned from GitHub

    git clone ssh://git@git.code.sintef.no/sidase/dlite.git

To initialize the minunit submodule, you may also have to run

    git submodule update --init

### Pre-build docker container
Docker containers are available from
[https://github.com/SINTEF/dlite/packages][dlite-packages].

Dependencies
------------

### Runtime dependencies
  - [HDF5][3], optional (needed by HDF5 storage plugin)
  - [Jansson][4], optional (needed by JSON storage plugin)
  - [Python 3][5], optional (needed by Python bindings and some plugins)
    - [NumPy][6], required if Python is enabled
    - [PyYAML][7], optional (used for generic YAML storage plugin)
    - [psycopg2][8], optional (used for generic PostgreSQL storage plugin)


### Build dependencies
  - [cmake][9], required for building
  - hdf5 development libraries, optional (needed by HDF5 storage plugin)
  - Jansson development libraries, optional (needed by JSON storage plugin)
  - Python 3 development libraries, optional (needed by Python bindings)
  - NumPy development libraries, optional (needed by Python bindings)
  - [SWIG v3][10], optional (needed by building Python bindings)
  - [Doxygen][11], optional, used for documentation generation
  - [valgrind][12], optional, used for memory checking (Linux only)
  - [cppcheck][13], optional, used for static code analysis


Compiling
---------
See [here](doc/build-with-vs.md) for instructions for building with
Visual Studio.


### Quick start with VS Code and Remote Container

Using Visual Studio Code it is possible to do development on the
system defined in Dockerfile.

1. Download and install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the extension __Remote Development__.
3. Clone _dlite_ and initialize git modules: `git submodule update --init`.
4. Open the _dlite_ folder with VS Code.
5. Start VS Code, run the *Remote-Containers: Open Folder in
   Container...* command from the Command Palette (F1) or quick
   actions Status bar item. This will build the container and restart
   VS Code in it. This may take some time the first time as the Docker
   image must be built. See [Quick start: Open an existing folder in a
   container][vs-container] for more information and instructions.
6. In the container terminal, perform the first build and tests with
   `mkdir /workspace/build; cd /workspace/build; cmake ../dlite; make &&
   make test`.


### Build on Linux

Install the hdf5 (does not include the parallel component) and jansson
libraries

On Ubuntu:

    sudo apt-get install libhdf5-serial-dev libjansson-dev

On Redhad-based distributions (Fedora, Centos, ...):

    sudo dnf install hdf5-devel jansson-devel

Build with:

    mkdir build
    cd build
    cmake ..
    make

Before running make, you may wish to configure some options with
`ccmake ..`

For example, you might need to change CMAKE_INSTALL_PREFIX to a
location accessible for writing. Default is ~/.local

To run the tests, do

    make test        # same as running `ctest`
    make memcheck    # runs all tests with memory checking (requires
                     # valgrind)

To generate code documentation, do

    make doc         # direct your browser to build/doc/html/index.html

To install dlite locally, do

    make install



Using dlite
===========
If dlite is installed in a non-default location, you may need to set
the PATH, LD_LIBRARY_PATH, PYTHONPATH and DLITE_ROOT environment
variables.  See the [documentation of environment
variables](doc/environment-variables.md) for more details.

For how to link to dlite from C, see the examples in the
[examples](examples) directory.

---

DLite is developed with the hope that it will be a delight to work with.

[1]: https://stash.code.sintef.no/projects/SOFT/repos/soft5/
[2]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
[3]: https://support.hdfgroup.org/HDF5/
[4]: http://www.digip.org/jansson/
[5]: https://www.python.org/
[6]: https://pypi.org/project/numpy/
[7]: https://pypi.org/project/PyYAML/
[8]: https://pypi.org/project/psycopg2/
[9]: https://cmake.org/
[10]: http://www.swig.org/
[11]: http://www.doxygen.org/
[12]: http://valgrind.org/
[13]: http://cppcheck.sourceforge.net/
[SOFT5_nomenclauture]: https://confluence.code.sintef.no/display/SOFT/Nomenclature
[UUID]: https://en.wikipedia.org/wiki/Universally_unique_identifier
[URL]: https://en.wikipedia.org/wiki/URL
[URI]: https://en.wikipedia.org/wiki/Uniform_Resource_Identifier
[IRI]: https://en.wikipedia.org/wiki/Internationalized_Resource_Identifier
[dlite-packages]: https://github.com/SINTEF/dlite/packages
[vs-container]: https://code.visualstudio.com/docs/remote/containers#_quick-start-open-an-existing-folder-in-a-container
