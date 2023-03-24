CMake options
=============
DLite support the following CMake options:

| **CMake option** | **Default** | **Comment**                        |
|:---------------- | -----------:|:---------------------------------- |
| `WITH_Python`    |          ON | Whether to build Python 3 bindings |
| `WITH_FORTRAN`   |         OFF | Whether to build Fortran bindings  |
| `WITH_HDF5`      |          ON | Whether to build with support for HDF5 v1.10+ |
| `WITH_REDLAND`   |          ON | Whether to build with RDF support (using librdf)|
| `WITH_DOC`       |         OFF | Whether to build documentation |
| `WITH_EXAMPLES`  |          ON | Whether to build/run examples during testing |
| `FORCE_EXAMPLES` |         OFF | Whether to force building/running examples |
| `ALLOW_WARNINGS` |          ON | Whether to not fail on compilation warnings |
| `WITH_THREADS`   |         OFF | Whether to build with threading support |
| `WITH_STATIC_PYTHON` |     OFF | Whether to compile with static python libraries |


Other common [CMake] options that you might to set (see [CMake variables] for a complete list):

| **CMake option** | **Default** | **Comment**                        |
|:---------------- | -----------:|:---------------------------------- |
| `CMAKE_INSTALL_PREFIX`|~/.local| Install path prefix, prepended onto install directories |
| `CMAKE_BUILD_TYPE`|      Debug | Build type: Debug, Release, MinSizeRel or RelWithDebInfo |
| `BUILD_TESTING`  |          ON | Whether to build with test support |


[CMake]: https://cmake.org/
[CMake variables]: https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html
