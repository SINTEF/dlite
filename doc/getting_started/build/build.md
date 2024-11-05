Build from source
=================
Please install the [build dependencies] you want to compile DLite against before compiling from source.

The source code can be cloned from GitHub

    git clone https://github.com/SINTEF/DLite.git


Please see the separate instructions if you want to [build with Visual Studio].


Configuration
-------------
DLite uses [CMake] as build system and supports many platforms.
Create a build directory and configure CMake

    mkdir build
    cd build
    cmake [CONFIG_OPTIONS] ..

where `[CONFIG_OPTIONS]` are CMake configuration options.
Configuring CMake is normally done with the `-D` CMake option to specify [configuration variables].

For example, to build against Python 3.10 with HDF5 support, configure with

    cmake -DWITH_PYTHON=YES -DPYTHON_VERSION=3.10 -DWITH_HDF5=YES ..

Please refer to the [CMake documentation] for more options.


### Configure for Python environment
See [build against Python environment] for how to configure against a Python virtualenv.


Build & install
---------------
After configuring cmake, DLite can be built and installed with:

    cmake --build .
    cmake --install .


Testing
-------
To test DLite, install the [test dependencies] and run:

    ctest

If you have [valgrind] installed, you can run the tests with memory checking turned on:

    cmake --build . --target memcheck

For more info about checking and debugging memory issues, see [Memory debugging].


Build documentation
-------------------
In order to reduce build dependencies for the causal user, DLite does not build documentation by default.
Provide the `-DWITH_DOC=YES` option to `cmake` to build the documentation.

DLite uses [Sphinx] to generate documentation from Python source code.
Ensure the correct virtual environment is set up and install the requirements

    pip install -r requirements_doc.txt

If you have [Doxygen] installed, the HTML documentation of the C API should be generated as a part of the build process.
It can be browsed by opening the following file in your browser:

    <build>/doc/html/index.html

where `<build>` is your build folder.

To only build the documentation, you can do:

    cd build
    cmake --build . --target doc

If you have LaTeX and `make` installed, you can also the latex documentation with

    cd build
    cmake --build . --target latex

which will produce the file

    <build>/doc/latex/refman.pdf


Quick start with Visual Studio Code and Docker
----------------------------------------------
Using [Visual Studio Code] (VS Code) it is possible to do development on the system defined in [Dockerfile](https://github.com/SINTEF/dlite/blob/master/Dockerfile).

1. Download and install [Visual Studio Code].
2. Install the extension **Remote Development**.
3. [Clone DLite](#build-from-source)
4. Open the DLite folder with VS Code.
5. Start VS Code, run the _Remote-Containers: Open Folder in Container..._ command from the Command Palette (F1) or quick actions Status bar item.
   This will build the container and restart VS Code in it.
   This may take some time the first time as the Docker image must be built.
   See [Quick start: Open an existing folder in a container][vs-container] for more information and instructions.
6. In the container terminal, perform the first build and tests with

   ```shell
   mkdir /workspace/build
   cd /workspace/build
   cmake ../DLite
   cmake --build .
   ctest
   ```


[git]: https://git-scm.com/
[CMake]: https://cmake.org/
[CMake variables]: https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html
[CMake documentation]: https://cmake.org/cmake/help/latest/manual/cmake.1.html
[CTest]: https://cmake.org/cmake/help/latest/manual/ctest.1.html
[HDF5]: https://support.hdfgroup.org/HDF5/
[librdf]: https://librdf.org/
[Fortran]: https://en.wikipedia.org/wiki/Fortran
[Python]: https://www.python.org/
[NumPy]: https://pypi.org/project/numpy/
[SWIG]: https://www.swig.org/
[cppcheck]: http://cppcheck.sourceforge.net/
[Doxygen]: http://www.doxygen.org/
[Sphinx]: https://www.sphinx-doc.org/
[valgrind]: http://valgrind.org/
[Visual Studio Code]: https://code.visualstudio.com/
[vs-container]: https://code.visualstudio.com/docs/remote/containers#_quick-start-open-an-existing-folder-in-a-container
[VirtualEnvWrapper]: https://virtualenvwrapper.readthedocs.io/en/latest/
[tripper]: https://pypi.org/project/tripper/
[PyYAML]: https://pypi.org/project/PyYAML/
[psycopg2]: https://pypi.org/project/psycopg2/
[pymongo]: https://github.com/mongodb/mongo-python-driver
[mongomock]: https://github.com/mongomock/mongomock
[pandas]: https://pandas.pydata.org/
[build dependencies]: https://sintef.github.io/dlite/getting_started/build/build-dependencies.html
[test dependencies]: https://sintef.github.io/dlite/getting_started/build/build-dependencies.html#testing-optional
[runtime dependencies]: https://sintef.github.io/dlite/getting_started/build/runtime-dependencies.html
[configuration variables]: https://sintef.github.io/dlite/getting_started/build/cmake_variables.html
[build against Python environment]: https://sintef.github.io/dlite/getting_started/build/build_against_python_env.html
[build with Visual Studio]: https://sintef.github.io/dlite/getting_started/build/build_with_vs.html
[Memory debugging]: https://sintef.github.io/dlite/contributors_guide/tips_and_tricks.html#memory-debugging
