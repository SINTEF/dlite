CMake variables
===============
The table below shows the most common variables for building DLite (see [CMake variables] for a complete list):

| **CMake variable**    | **Default** | **Comment**                        |
|:----------------      | -----------:|:---------------------------------- |
| `CMAKE_INSTALL_PREFIX`|    ~/.local | Install path prefix, prepended onto install directories |
| `CMAKE_BUILD_TYPE`    |       Debug | Build type: Debug, Release, MinSizeRel or RelWithDebInfo |
| `WITH_Python`         |          ON | Whether to build Python 3 bindings |
| `PYTHON_VERSION`      |             | Python version to compile against.  Example: `3.11` |
| `WITH_FORTRAN`        |         OFF | Whether to build Fortran bindings  |
| `WITH_HDF5`           |          ON | Whether to build with support for HDF5 v1.10+ |
| `WITH_REDLAND`        |          ON | Whether to build with RDF support (using librdf)|
| `WITH_DOC`            |         OFF | Whether to build documentation |
| `WITH_EXAMPLES`       |          ON | Whether to build/run examples during testing |
| `FORCE_EXAMPLES`      |         OFF | Whether to force building/running examples |
| `ALLOW_WARNINGS`      |          ON | Whether to not fail on compilation warnings |
| `WITH_THREADS`        |         OFF | Whether to build with threading support |
| `WITH_STATIC_PYTHON`  |         OFF | Whether to compile with static python libraries |
| `BUILD_TESTING`       |          ON | Whether to build with test support |



[CMake variables]: https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html
