Build with Visual Studio
========================

1. Install "Visual Studio 15 2017" or "Visual Studio 16 2019" with the
   C/C++ components and cmake support. In case you want to build Python bindings,
   install "Visual Studio 15 2017" since this is the version, recent Pythons have
   been compiled with on Windows.

2. (Optional) Install SWIG in case you want to build python-bindings as described
   here [swig]. Add the swig-executable to your windows PATH.
   
3. (Optional) Check versions. Tested with the following

   $ swig -version
     SWIG Version 4.0.2
     Compiled with i686-w64-mingw32-g++ [i686-w64-mingw32]
     Configured options: +pcre
     Please see http://www.swig.org for reporting bugs and further information

   $ cmake -version
    cmake version 3.20.21032501-MSVC_2
    CMake suite maintained and supported by Kitware (kitware.com/cmake).


3. (Optional) Download hdf5 library archive from
   [https://support.hdfgroup.org/ftp/HDF5/current/src/][hdf5]

4. git clone dlite to a directory of your choice

5. Configure, build, install and test from the Windows comand line

   $ mkdir .\out\build\x64-Release
   $ cd .\out\build\x64-Release
   $ cmake -G "Visual Studio 15 2017" ^
           -A x64 ^
           -DWITH_DOC=OFF ^
           -DWITH_JSON=ON ^
           -DBUILD_JSON=ON ^
           -DWITH_HDF5=OFF ^
           -DCMAKE_CONFIGURATION_TYPES:STRING="Release" ^
           -DCMAKE_INSTALL_PREFIX:PATH="%CD%\..\..\install\x64-Release" ^
           ..\..\..

    $ cmake --build . --config Release
    $ cmake --install .
    $ ctest -C Release

6. Issues: 4 tests fail

   92% tests passed, 4 tests failed out of 48
   
   Total Test time (real) =   3.11 sec
   
   The following tests did not run:
            37 - test_postgresql_storage (Disabled)
            38 - test_postgresql_storage2 (Disabled)
   
   The following tests FAILED:
            34 - test_mapping (Failed)
            39 - test_yaml_storage (Failed)
            40 - test_python_bindings-py_venv_python3.8.5 (Failed)
            42 - test_entity-py_venv_python3.8.5 (Failed)


[cmake]: https://cmake.org/download/
[hdf5]: https://support.hdfgroup.org/ftp/HDF5/current/src/
[swig]: https://www.dev2qa.com/how-to-install-swig-on-macos-linux-and-windows/