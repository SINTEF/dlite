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

See [src/dlite.h](src/dlite.h) for an overview of the current api. So
far only reading and writing HDF5 files is implemented.

*dlite* also includes [uuid][3] (a small library for generating UUIDs)
as a git submodule.


Dependencies
------------
*dlite* has the following dependencies:
  - [cmake][4], required for building
  - [hdf5][5], required (cmake will automatically downloaded and built hdf5
    if it is not found)
  - [doxygen][6], optional, used for documentation generation
  - [valgrind][7], optional, used for memory checking (Linux only)


Download
--------
Download dlite with git, using

    git clone ssh://git@git.code.sintef.no/sidase/dlite.git

To initialize the uuid submodule, you may also have to run

    git submodule update --init


Building
--------

## Windows with Visual Studio

Create a "build" subfolder and run cmake:

    mkdir build
    cd build
    cmake ..

It is possible that you have to run cmake as
`cmake  -G "Visual Studio 11 2012 Win64" ..`.

Run or double-click on `dlite.sln`, which will open Visual Studio.


## Linux

Build with:

    mkdir build
    cd build
    cmake ..
    make

Before running make, you may wish to configure some options with
`ccmake ..`

To run the tests, do

    make test        # same as running `ctest`
    make memcheck    # runs all tests with memory checking (requires valgrind)

To generate code documentation, do

    make doc         # direct your browser to doc/html/index.html

To install dlite locally, do

    make install


The future of dlite
-------------------
Ideally dlite will be merged into SOFT when SOFT compiles well on Windows.
Until then, it will remain as a simple and mostly compatible alternative.


License
-------
For compatibility with SOFT, *dlite* is dual licensed under a
commercial and open source LGPL v2.1 license.

---

*dlite* is developed with the hope that it will be a delight to work with.

[1]: https://stash.code.sintef.no/projects/SOFT/repos/soft5/
[2]: https://github.com/NanoSim/Porto/blob/porto/Preview-Final-Release/doc/manual/02_soft_introduction.md#soft5-features
[3]: https://stash.code.sintef.no/projects/sidase/repos/uuid/
[4]: https://cmake.org/
[5]: https://support.hdfgroup.org/HDF5/
[6]: http://www.doxygen.org/
[7]: http://valgrind.org/
[8]: https://github.com/petervaro/sodyll
