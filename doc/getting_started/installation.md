Installing DLite
================
DLite is written C, but can be compiled with or without bindings to Python and Fortran and with or without support for [HDF5] and [librdf] storages.
A set of additional optional storages are available if compiled with Python support.
For a complete list, see [Runtime dependencies](#runtime-dependencies).


Installing with pip
-------------------
If you are using Python, the easiest way to install DLite is with pip:

```shell
pip install DLite-Python
```

This will give you DLite together with all optional Python dependencies (see [Runtime dependencies](#runtime-dependencies)).


Installing from source
----------------------
Please install the [build dependencies](#build-dependencies) as well as the optional [runtime dependencies](#runtime-dependencies) you want to compile DLite against before compiling from source.

The source code can be cloned from GitHub

    git clone https://github.com/SINTEF/DLite.git

DLite uses [CMake] as build system.
Building on a Linux system, may be as simple as the following:

    mkdir build
    cd build
    cmake ..              # configure
    cmake --build .       # build
    cmake --install .     # install
    ctest                 # test

CMake offers a large range of configuration options.
The table below shows the most common options for building DLite (see [CMake variables] for a complete list):

| **CMake option**      | **Default** | **Comment**                        |
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

For example, to build against Python 3.10 with HDF5 support, configure with

    cmake -DWITH_PYTHON=YES -DPYTHON_VERSION=3.10 -DWITH_HDF5=YES ..

Please refer to the [CMake documentation] for more options.

If you have [valgrind] installed, you can run all with memory checking turned on, with

    make memcheck


### Build and install to a virtual Python environment
This is the most common case for a development installation on a Linux environment (including Ubuntu from WSL).

[virtualenvwrapper] is a set of convenient tools for working with virtual environments in Python.

:::{admonition} Note about VirtualEnvWrapper
:class: note

By default, [VirtualEnvWrapper] does not set `LD_LIBRARY_PATH`.
This will result in errors when running, for example, `dlite-codegen` in the example above.
To fix this, after compiling and installing `dlite`, the user needs prepend/append `$VIRTUAL_ENV/lib/` to `LD_LIBRARY_PATH`.
This can be done by modifying the `activate` shell file, located at `$WORKON_HOME/<envs_name>/bin/activate`. First, the user should add

``` bash
if ! [ -z "${_OLD_LD_LIBRARY_PATH}" ] ; then
    LD_LIBRARY_PATH="$_OLD_LD_LIBRARY_PATH"
    export LD_LIBRARY_PATH
    unset _OLD_LD_LIBRARY_PATH
fi
```
at the end of the `deactivate` function in the `activate` shell file. Next, add
``` bash
export _OLD_LD_LIBRARY_PATH=$LD_LIBRARY_PATH
export LD_LIBRARY_PATH="$VIRTUAL_ENV/lib/:$LD_LIBRARY_PATH"
```
at the end of `activate`.

**Explanation:** The value of `LD_LIBRARY_PATH` is exported (saved) into a new temporary environment variable, `_OLD_LD_LIBRARY_PATH`.
`$VIRTUAL_ENV/lib/` is then prepended to `LD_LIBRARY_PATH`.
The `if` statement within the `deactivate` function checks whether the variable `_OLD_LD_LIBRARY_PATH` has been declared.
If true, then the `deactivate` function will set `LD_LIBRARY_PATH` to its original value and unset the temporary environment variable `_OLD_LD_LIBRARY_PATH`.
:::

Using virtualenvwrapper, compiling DLite against Python 3.10, may look like:

    pip install --user virtualenvwrapper
    export WORKON_HOME=$HOME/.envs  # Put this in your ~/.bash_profile
    mkvirtualenv -p /usr/bin/python3.10 dlite  # The -p option allows you to select Python executable
    pip install -U pip
    pip install -r requirements.txt
    pip install -r requirements_dev.txt
    pip install -r requirements_doc.txt

    workon dlite  #  only needed if you are not already in your dlite virtualenv
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=$WORKON_HOME/dlite -DPYTHON_VERSION=3.10 ..
    cmake --build .
    cmake --install .
    ctest


### Build documentation
In order to reduce build dependencies for the causal user, DLite does not build documentation by default.
Provide the ``-DWITH_DOC=YES`` option to ``cmake`` to build the documentation.

DLite uses [Sphinx] to generate documentation from Python source code.
Ensure the correct virtual environment is set up and install the requirements

    pip install -r requirements_doc.txt

If you have [Doxygen] installed, the HTML documentation of the C API should be generated as a part of the build process.
It can be browsed by opening the following file in your browser:

    <build>/doc/html/index.html

where ``<build>`` is your build folder.

To only build the documentation, you can do:

    cd build
    cmake --build . --target doc

If you have LaTeX and ``make`` installed, you can also the latex documentation with

    cd build
    cmake --build . --target latex

which will produce the file

    <build>/doc/latex/refman.pdf


### Build with Visual Studio
Please see the separate instructions for [building with Visual Studio](https://sintef.github.io/dlite/getting_started/build/build_with_vs.html).


### Quick start with Visual Studio Code and Docker
Using [Visual Studio Code] (VS Code) it is possible to do development on the sys
tem defined in [Dockerfile](https://github.com/SINTEF/dlite/blob/master/Dockerfi
le).

1. Download and install [Visual Studio Code].
2. Install the extension **Remote Development**.
3. [Clone DLite](#installing-from-source)
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


Dependencies
------------

### Build dependencies
For a basic build, at least the following is required:
- C compiler and standard C development libraries (ex: `sudo apt install build-essential`)
- [CMake] (ex `sudo apt install cmake`)
- [git] (ex `sudo apt install git`)

For configuring [CMake], the `ccmake` tool is handy.  Install it with `sudo apt install cmake-curses-gui`.  Run it as `ccmake .` from the build directory.

#### Python bindings (optional)
For Python bindings, you need Python with development libraries, [SWIG] and [NumPy] installed.
The following example shows how to install Python 3.10 on Ubuntu (which currently comes with Python 3.8) together with development libraries:

    sudo add-apt-repository ppa:deadsnakes/ppa -y
    sudo apt install python3.10 python3.10-dev python3.10-distutils swig
    pip install numpy  # you might want to do this from a virtual environemnt

#### Fortran bindings (optional)
For Fortran bindings, you need Fortran development libraries.
On Ubuntu, they can be installed with

    sudo apt install gfortran

#### Redland librdf support (optional)
Redland [librdf] is a set of C libraries providing support for RDF.
It is currently used by the rdf storage plugin.

    sudo apt install librdf-dev

#### HDF5 support (optional)
[HDF5] development development libraries are used by the hdf5 storage plugin.
On Ubuntu, they can be installed with

    sudo apt install libhdf5-dev


### Runtime dependencies (optional)
If DLite has been compiled with Python, Fortran, librdf or HDF5 support, you will need the corresponding runtime libraries.
These are normally installed together with the corresponding development libraries.
But if you are distributing DLite, the corresponding runtime libraries will have to be installed separately.

When DLite is compiled with Python bindings, additional runtime features may be enabled by installing one of more of the following optional Python packages
- [tripper], optional, (used for property mappings)
- [PyYAML], optional (used for generic YAML storage plugin)
- [psycopg2], optional (used for generic PostgreSQL storage plugin)
    Note that in some cases a GSSAPI error is raised when using psycopg2
    by pip installing psycopg2-binary.
    This is solved by installing from source as described in their documentation.
- [pandas], optional (used for csv storage plugin)
- [pymongo], optional, (used for mongodb storage plugin)
- [mongomock], optional, used for testing mongodb storage plugin.

These dependencies can be installed with

    pip install -r requirements_dev.txt


### Test dependencies (optional)
DLite uses [cppcheck] for static code analysis and [valgrind] for memory checking.
On Ubuntu they can be installed with

    sudo apt install cppcheck valgrind


### Dependencies for building documentation (optional)
DLite uses [Sphinx] and [Doxygen] to generate documentation.
Ubuntu they can be installed with

    sudo apt install cppcheck Doxygen
    pip install -r requirements_doc.txt



[git]: https://git-scm.com/
[CMake]: https://cmake.org/
[CMake variables]: https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html
[CMake documentation]: https://cmake.org/cmake/help/latest/manual/cmake.1.html
[CTest]: https://cmake.org/cmake/help/latest/manual/ctest.1.html
[HDF5]: https://support.hdfgroup.org/HDF5/
[librdf]: https://librdf.org/
[Python 3]: https://www.python.org/
[NumPy]: https://pypi.org/project/numpy/
[SWIG]: https://www.swig.org/
[virtualenvwrapper]: https://pypi.org/project/virtualenvwrapper/
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
