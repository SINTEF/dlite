dlite -- lightweight library for working with scientific data
=============================================================
*dlite* is a small cross-platform C library under development, for
working with and sharing scientific data in an interoperable way.  It
is strongly inspired by [SOFT][1], with the aim to be a lightweight
replacement in cases where Windows portability is a showstopper for
using SOFT.

*dlite* shares the [metadata model of SOFT][2] and generic data stored
with SOFT can be read with dlite and vice verse.  However, apart from
*dlite* being much less complete, there are also some differences.
See [doc/concepts.md](doc/concepts.md) for details.

The main concepts and components of *dlite* includes:
  - Instance: a formal representation of data
  - Metadata: describes an instance (metadata are instances themselves)
  - Collection: a specialised instance that contains references to set
    of instances and relations between them
  - Storage: a generic handle encapsulating actual storage formats via
    plugins


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
Download dlite with git, using

    git clone ssh://git@git.code.sintef.no/sidase/dlite.git

To initialize the uuid submodule, you may also have to run

    git submodule update --init


Building
--------

## Build on Microsoft Windows

1. Install a recent version of cmake https://cmake.org/download/
2. Install Visual Studio 14 2015 or Visual Studio 15 2017 with the
   C/C++ components

3. Prepare a directory structure for dlite and 3rd party libraries:
   1. Select a root folder to create the directory structure
      (e.g. "C:\" or "C:\Users\{username}\Documents\")
	2. Create the following directory: {root}\local
	3. Clone the dlite repository in the root folder, folder
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


Summary to build and install dlite when hdf5 and jansson lib are installed
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


The future of dlite
-------------------
Ideally dlite will be merged into SOFT when SOFT compiles well on Windows.
Until then, it will remain as a simple and mostly compatible alternative.


---

*dlite* is developed with the hope that it will be a delight to work with.

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
