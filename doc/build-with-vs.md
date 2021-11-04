Build with Visual Studio
========================

1. Install "Visual Studio 14 2015", "Visual Studio 15 2017" or "Visual Studio 16 2019" with the
   C/C++ components and cmake.

   In case you want Python-bindings, make sure to use a "Visual Studio" version
   that generates dlite-libraries which are compatible to the Python version you are using.
   For Python>=3.5, any of the above versions will work: See [mscompilers] for a discussion on
   binary compatibilty between Visual Studio compiler versions. For older versions of Python see
   the correct versions here: [pythonwindows].

   In the examples below, we use "Visual Studio 15 2017" which is the compiler version with which
   Python>=3.7 is build on Windows.

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

5. Build and test

5.1. Build from the Windows commandline

   When building with Python support, this will build against your default Python.
   Make sure you have the following Python packages installed: numpy, pyyaml, pandas, psycopg2
   Use `py -0p` to check the Python default.

   $ mkdir .\out\build\x64-Release-vs15
   $ cd .\out\build\x64-Release-vs15
   $ cmake -G "Visual Studio 15 2017" ^
           -A x64 ^
           -DWITH_DOC=OFF ^
           -DWITH_HDF5=OFF ^
           -DCMAKE_CONFIGURATION_TYPES:STRING="Release" ^
           -DCMAKE_INSTALL_PREFIX:PATH="%CD%\..\..\install\x64-Release-vs15" ^
           ..\..\..

    $ cmake --build . --config Release
    $ cmake --install .
    $ ctest -C Release

5.2 Build from Visual Studio

   When building with Python support, this will again build against your default Python.

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
        }
      ]
    }

    * Select the "x64-Release-vs15" configuration
    * Right click on the top-level "CMakeLists.txt" -> "configure dlite"
    * Right click on the top-level "CMakeLists.txt" -> "Build"
    * Right click on the top-level "CMakeLists.txt" -> "Run Tests"

5.3 Build against Python in a virtual environment (Recommended)

   This example uses Anaconda Python and conda environemnts.

   $ conda create --name=py38dlite python=3.8 numpy pyyaml pandas psycopg2
   $ conda activate py38dlite

   $ mkdir .\out\build\x64-Release-vs15
   $ cd .\out\build\x64-Release-vs15
   $ cmake -G "Visual Studio 15 2017" ^
           -A x64 ^
           -DWITH_DOC=OFF ^
           -DWITH_HDF5=OFF ^
           -DCMAKE_CONFIGURATION_TYPES:STRING="Release" ^
           -DCMAKE_INSTALL_PREFIX:PATH="%CD%\..\..\install\x64-Release-vs15" ^
           ..\..\..

    $ cmake --build . --config Release
    $ cmake --install .
    $ ctest -C Release

6. Install the Python package via using ./python/setup.py from sources

   $ conda create --name=py38dlite python=3.8 numpy
   $ conda activate py38dlite
   $ cd python
   $ python setup.py install

   # Or use the following
   $ conda create --name=py38dlite python=3.8
   $ conda activate py38dlite
   $ cd python
   # --use-feature=in-tree-build is required for pip < 21.3. for pip >= 21.3 it will become the default.
   $ pip --use-feature=in-tree-build install .


7. Install dlite-python via a pre-packaged wheel

   $ conda create --name=py37dlite python=3.7 numpy
   $ conda activate py37dlite
   $ pip install dlite_python-0.3.3-cp37-cp37m-win_amd64.whl



[cmake]: https://cmake.org/download/
[hdf5]: https://support.hdfgroup.org/ftp/HDF5/current/src/
[swig]: https://www.dev2qa.com/how-to-install-swig-on-macos-linux-and-windows/
[pythonwindows]: https://pythondev.readthedocs.io/windows.html
[mscompilers] https://docs.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017

