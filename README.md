DLite -- lightweight library for working with scientific data
=============================================================
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
  - Simple and structured way to represent data as a set of named properties
    within and between software
  - Simple type system where data type are specified as a basic type and size
    Supported basic types includes:
      - binary blob (any size)
      - boolean
      - integer (8, 16, 32, 64 bits)
      - unsigned integer (8, 16, 32, 64 bits)
      - float (32, 64, [80, 128] bits)
      - fixed string (any size, always NUL-terminated)
      - string pointer
      - relation
      - dimension (only intended for metadata)
      - property (only intended for metadata)
  - Supports units and multi-dimensional arrays
  - Fully implemented metadata model as presented by Thomas Hagelien
  - Builtin HDF5 and JSON storage plugins
  - Plugin system for user-provided storage drivers
  - Memory for metadata and instances is reference counted
  - Lookup of metadata and instances via global paths (initiated from the
    DLITE_STORAGES environment variable)
  - Template-based code generation (includes templates for C, Fortran
    templates are planned)
  - Plugin system for mappings that maps instances of a set of input metadata
    to an output instance
  - Python bindings
  - Fortran bindings (planned)
  - Storage and mapping plugins written in Python (planned)


Short vocabulary
----------------
  - **Basic metadata schema**: Toplevel meta-metadata which describes itself.
  - **Collection**: A specialised instance that contains references to set
    of instances and relations between them.  Within a collection instances
    are labeled.  See also the [SOFT5 nomenclauture][SOFT5_nomenclauture].
  - **Data instance**: A "leaf" instance that is not metadata.
  - **Entity**: A special type of metadata that describes standard data
    instances.  This is different from SOFT5 where entities are the
    fundamental metadata.
  - **Instance**: The basic data object in DLite.  All instances are described
    by their metadata which itself are instances.  Instances are identified
    by an UUID.
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



Runtime dependencies
--------------------
  - [HDF5][3], optional (needed by HDF5 storage plugin)
  - [Jansson][4], optional (needed by JSON storage plugin)
  - [Python][5], optional (needed by Python bindings and some plugins)
  - [NumPy][6], optional (needed by Python bindings and some plugins)


Build dependencies
------------------
  - [cmake][7], required for building
  - hdf5 development libraries, optional (needed by HDF5 storage plugin)
  - Jansson development libraries, optional (needed by JSON storage plugin)
  - python development libraries, optional (needed by Python bindings)
  - NumPy development libraries][8], optional (needed by Python bindings)
  - SWIG[8], optional (needed by building Python bindings)
  - Doxygen][9], optional, used for documentation generation
  - [valgrind][10], optional, used for memory checking (Linux only)


Download
--------
Download DLite with git, using

    git clone ssh://git@git.code.sintef.no/sidase/dlite.git

To initialize the uuid submodule, you may also have to run

    git submodule update --init


Building
--------

## Build on Microsoft Windows

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


The future of DLite
-------------------
Ideally DLite will be merged into SOFT when SOFT compiles well on Windows.
Until then, it will remain as a simple and mostly compatible alternative.


---

DLite is developed with the hope that it will be a delight to work with.

[1]: https://stash.code.sintef.no/projects/SOFT/repos/soft5/
[2]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
[3]: https://support.hdfgroup.org/HDF5/
[4]: http://www.digip.org/jansson/
[5]: https://www.python.org/
[6]: http://www.numpy.org/
[7]: https://cmake.org/
[8]: http://www.swig.org/
[9]: http://www.doxygen.org/
[10]: http://valgrind.org/
[SOFT5_nomenclauture]: https://confluence.code.sintef.no/display/SOFT/Nomenclature
