dlite -- lightweight library for working with scientific data
=============================================================
*dlite* is a small cross-platform C library under development, for
working with and sharing scientific data in an interoperable way.  It
is strongly inspired by [SOFT][1], with the aim to be a lightweight
replacement in cases where Windows portability is a showstopper for
using SOFT.

*dlite* shares the metadata model of SOFT and generic data stored
with SOFT can be read with dlite and vice verse.

See [src/dlite.h](src/dlite.h) for a description of the current
api. So far only reading and writing HDF5 files is implemented.

*dlite* also includes [uuid][2] -- a small library for generating UUIDs.


Dependencies
------------
*dlite* has the following dependencies:
  - [cmake][3], required for building
  - [hdf5][4], required (cmake will automatically downloaded and built hdf5
    if it is not found)
  - [doxygen][5], optional, used for documentation generation
  - [valgrind][6], optional, used for memory checking (Linux only)


Building
--------
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

TODO: add instructions for building with Visual Studio on Windows.


Further plans
-------------
Depending on the adoption of SOFT, dlite may either be merged into
SOFT or remain as a lightweight portable alternative.  In the latter
case, possible new features could include:
  - code generator
  - plugin-system, e.g. based on [c-pluff][7]
  - more drivers, like JSON

---

*dlite* is developed with the hope that it will be a delight to work with.

[1]: https://stash.code.sintef.no/projects/SOFT/repos/soft5/
[2]: https://stash.code.sintef.no/projects/PRECIMS/repos/uuid/
[3]: https://cmake.org/
[4]: https://support.hdfgroup.org/HDF5/
[5]: http://www.doxygen.org/
[6]: http://valgrind.org/
[7]: https://github.com/jlehtine/c-pluff
