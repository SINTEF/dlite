Build with Visual Studio
========================

1. Install Visual Studio 15 2017 or Visual Studio 16 2019 with the
   C/C++ components and cmake.
   
   In case you want Python-bindings, use "Visual Studio 15 2017" for Python>=3.7. For older versions
   of Python see the correct versions here: [pythonwindows]

2. (Optional) Install SWIG in case you want to build Python-bindings as described
   here [swig]. Add the swig-executable to your windows PATH
   
3. (Optional) Check versions

   $ swig -version
     SWIG Version 4.0.2
     Compiled with i686-w64-mingw32-g++ [i686-w64-mingw32]
     Configured options: +pcre
     Please see http://www.swig.org for reporting bugs and further information

   $ cmake -version
    cmake version 3.20.21032501-MSVC_2
    CMake suite maintained and supported by Kitware (kitware.com/cmake).

4. git clone dlite to a directory of your choice

5. Build from the Windows commandline

   $ mkdir .\out\build\x64-Release-vs15
   $ cd .\out\build\x64-Release-vs15
   $ cmake -G "Visual Studio 15 2017" ^
           -A x64 ^
           -DWITH_DOC=OFF ^
           -DWITH_HDF5=OFF ^
           -DCMAKE_CONFIGURATION_TYPES:STRING="Release" ^
           -DCMAKE_INSTALL_PREFIX:PATH="%CD%\..\..\install\x64-Release" ^
           ..\..\..

    $ cmake --build . --config Release
    $ cmake --install .
    $ ctest -C Release

6. (Alternative) Build from Visual Studio

   Open the dlite directory with Visual Studio, it will be recognized as a CMake project.
   Add a CMakeSettings.json file with the following minimum contents:

    {
      "configurations": [
        {
          "name": "x64-Release-vs15",
          "generator": "Visual Studio 15 2017 Win64",
          "configurationType": "Release",
          "buildRoot": "${projectDir}\\out\\build\\${name}",
          "installRoot": "${projectDir}\\out\\install\\${name}",
          "cmakeCommandArgs": "-DWITH_DOC=OFF -DWITH_HDF5=OFF",
          "buildCommandArgs": "",
          "inheritEnvironments": [ "msvc_x64_x64" ]
        },
      ]
    }

    * Select the "x64-Release-vs15" configuration
    * Right click on the top-level "CMakeLists.txt" -> "configure dlite"
    * Right click on the top-level "CMakeLists.txt" -> "Build"
    * Right click on the top-level "CMakeLists.txt" -> "Run Tests"


[cmake]: https://cmake.org/download/
[hdf5]: https://support.hdfgroup.org/ftp/HDF5/current/src/
[swig]: https://www.dev2qa.com/how-to-install-swig-on-macos-linux-and-windows/
[pythonwindows]: https://pythondev.readthedocs.io/windows.html