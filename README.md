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


Building
--------
Build with:

    mkdir build
    cd build
    cmake ..
    make


Further plans
-------------
Depending on the adoption of SOFT, dlite may either be merged into
SOFT or remain as a lightweight portable alternative.  In the latter
case, possible new features could include:
  - code generator
  - plugin-system, e.g. based on [c-pluff][2]
  - more drivers, like JSON

---

*dlite* is developed with the hope that it will be a delight to work with.

[1]: https://stash.code.sintef.no/projects/SOFT/repos/soft5/
[2]: https://github.com/jlehtine/c-pluff
