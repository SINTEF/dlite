DLite - lightweight data-centric framework for working with scientific data
===========================================================================

![CI tests](https://github.com/sintef/dlite/workflows/CI%20tests/badge.svg)


DLite is a small cross-platform C library under development, for
working with and sharing scientific data in an interoperable way.  It
is strongly inspired by [SOFT][1], with the aim to be a lightweight
replacement in cases where Windows portability is a showstopper for
using SOFT.

DLite shares the [metadata model of SOFT5][2] and is compatible with
SOFT5 in many respects.  However, it has also some notable
differences, mainly with respect to the type system and that it fully
implements the metadata model envisioned in SOFT5.
See [doc/concepts.md](doc/concepts.md) for details.

DLite is licensed under the MIT license.


Main features
-------------
See [doc/features.md](doc/features.md) for a more detailed list.
  - Enables semantic interoperability via simple formalised metadata and data
  - Metadta can be linked to or generated from ontologies
  - Code generation for simple integration in existing code bases
  - Plugin API for data storages
  - Plugin API for mapping between metadata
  - Bindings to C, Python and Fortran


Example use
-----------
Lets say that you have the following Python class

```python
class Person:
    def __init__(self, name, age, skills):
        self.name = name
        self.age = age
        self.skills = skills

    def __repr__(self):
        return 'Person(%r, %r, %r)' % (self.name, self.age, list(self.skills))
```

that you want to describe semantically.  We do that by defining the
following metadata specifying `name` as a string, `age` as a float and
`skills` as an array of strings:

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
      "unit": "years",
      "description": "Age of person."
    },
    {
      "name": "skills",
      "type": "string",
      "dims": [
        "N"
      ],
      "description": "List of skills."
    }
  ]
}
```

and save it as "Person.json".  Back in Python we can now make a dlite-aware
version of the `Person` class, create an instance and serialise it to
a storage:

```python
import dlite

DLitePerson = dlite.classfactory(Person, url='json://Person.json')

person = DLitePerson('Sherlock Holmes', 34., ['observing', 'chemistry',
    'violin', 'boxing'])

print(person.dlite_inst.asjson(indent=2))
person.dlite_inst.save('json://homes.json?mode=w')
```

To access this instance from C, you can first generate a header file from
the meta data

```sh
dlite-codegen -f c-header -o person.h Person.json
```

and then include it in your C program:

```c
// homes.c
#include <stdio.h>
#include <dlite>
#include "person.h"

int main()
{
  int i;
  char *url = "json://homes.json#7ac977ce-a0dc-4e19-a7e1-7781c0cd23d2";

  Person *person = dlite_instance_load_url(url);

  printf("name:  %s\n", person->name);
  printf("age:   %g\n", person->age);
  printf("skills:\n");
  for (i=0; i<person->N; i++)
    printf("  - %s\n", person->skills[i]);

  return 0;
}
```

Since we are using `dlite_instance_load_url()` to load the instance,
you must link to dlite when compiling this program.  Assuming you are using
Linux and dlite in installed in `$HOME/.local`, compiling with gcc,
would look like:

```sh
gcc -g -ldlite -I$HOME/.local/include/dlite -L$HOME/.local/lib -o homes homes.c
```

Finally you can run the program with

```sh
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
'Person.json'.  There are ways to avoid that, either by hardcoding the
metadata in C using `dlite-codegen -f c-source` or in the C program
explicitely load Person.json before homes.json.


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

Quick start with VS Code and Remote Container
---------------------------------------------

Using Visual Studio Code it is possible to do development on the system defined in Dockerfile.

1. Download and install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the extension __Remote Development__.
3. Clone _dlite_ and initialize git modules: `git submodule update --init`.
4. Open the _dlite_ folder with VS Code.
5. Start VS Code, run the *Remote-Containers: Open Folder in Container...* command from the Command Palette (F1) or quick actions Status bar item. This will build the container and restart VS Code in it. This may take some time the first time as the Docker image must be built. See [Quick start: Open an existing folder in a container](https://code.visualstudio.com/docs/remote/containers#_quick-start-open-an-existing-folder-in-a-container) for more information and instructions.
6. In the container terminal, perform the first build and tests with `mkdir /workspace/build; cd /workspace/build; cmake ../dlite; make && make test`.


Docker container
----------------
A docker containiner can be found on
[https://github.com/SINTEF/dlite/packages](https://github.com/SINTEF/dlite/packages).


Runtime dependencies
--------------------
  - [HDF5][3], optional (needed by HDF5 storage plugin)
  - [Jansson][4], optional (needed by JSON storage plugin)
  - [Python 3][5], optional (needed by Python bindings and some plugins)
    - [NumPy][6], required if Python is enabled
    - [PyYAML][7], optional (used for generic YAML storage plugin)
    - [psycopg2][8], optional (used for generic PostgreSQL storage plugin)


Build dependencies
------------------
  - [cmake][9], required for building
  - hdf5 development libraries, optional (needed by HDF5 storage plugin)
  - Jansson development libraries, optional (needed by JSON storage plugin)
  - Python 3 development libraries, optional (needed by Python bindings)
  - NumPy development libraries, optional (needed by Python bindings)
  - [SWIG v3][10], optional (needed by building Python bindings)
  - [Doxygen][11], optional, used for documentation generation
  - [valgrind][12], optional, used for memory checking (Linux only)
  - [cppcheck][13], optional, used for static code analysis


Download
--------
Download DLite with git, using

    git clone ssh://git@git.code.sintef.no/sidase/dlite.git

To initialize the minunit submodule, you may also have to run

    git submodule update --init


Building
--------

## Build on Microsoft Windows

@verbatim
1. Install a recent version of cmake https://cmake.org/download/
2. Install Visual Studio 14 2015 or Visual Studio 15 2017 with the
   C/C++ components

3. Prepare a directory structure for DLite and 3rd party libraries:
   1. Select a root folder to create the directory structure
      (e.g. "C:\" or "C:\Users\{username}\Documents\")
	2. Create the following directory: {root}\local
	3. Clone the DLite repository in the root folder, folder
           {root}\dlite will be created.
4. Download hdf5 library archive from
   https://support.hdfgroup.org/ftp/HDF5/current/src/
5. Make a copy of the file {root}\dlite\bootstrap-win.sh into the root
   folder
6. Edit the copy of bootstrap-win.sh in the root folder:
	1. Modify the variable CMAKE_PATH to the path of cmake directory
	2. Modify the variable ROOT_PATH to your root folder
	3. Modify the variable PYTHON_EXECUTABLE to Python version 3.x
	   executable
	4. Check the version number of hdf5 library
    5. To build in Win64 mode, add '-G "Visual Studio 15 Win64"' after
       cmake command when generating the solution (not building)
7. Open a git-bash window:
	1. Change the directory to your root folder
	2. Type `sh bootstrap-win.sh` and press enter
8. Open the file {root}\dlite\build\dlite.sln in Visual Studio
9. In Visual Studio:
    1. Select the solution configuration "Debug", then build the
       solution (Menu Build -> Build solution)
    2. Select the solution configuration "Release", then build the
       solution (Menu Build -> Build solution)

To run the tests, do

    ctest -C Debug


Summary to build and install DLite when hdf5 and jansson lib are installed
in the given path LOCAL_DIR

	LOCAL_DIR=/C/Users/tco/Documents/Programs/philib/local
	PATH=$PATH:$LOCAL_DIR/bin
	cd dlite
	mkdir build && cd build
	cmake -G "Visual Studio 14 2015 Win64" -DHDF5_DIR=$LOCAL_DIR/cmake/hdf5 -DJANSSON_ROOT=$LOCAL_DIR/ -DCMAKE_INSTALL_PREFIX=$LOCAL_DIR/ ..
	cmake --build . --config Debug --target doc
	cmake --build . --config Debug --target install
	cmake --build . --config Release --target install
	ctest -C Debug
	ctest -C Release

@endverbatim


## Build on Linux

If hdf5 is not installed (does not include the parallel component):

    sudo apt-get install libhdf5-serial-dev

If JANSSON is not installed:

    sudo apt-get install libjansson-dev

Build with:

    mkdir build
    cd build
    cmake ..
    make

Before running make, you may wish to configure some options with
`ccmake ..`

For example, you might need to change (using e.g. cmake-gui)
CMAKE_INSTALL_PREFIX to a location accessible for writing. Default
is ~/.local


To run the tests, do

    make test        # same as running `ctest`
    make memcheck    # runs all tests with memory checking (requires
                     # valgrind)

To generate code documentation, do

    make doc         # direct your browser to build/doc/html/index.html

To install dlite locally, do

    make install


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
