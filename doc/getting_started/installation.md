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

DLite uses [CMake] as build system.
Building on a Linux system, may be as simple as the following:

    mkdir build
    cd build
    cmake ..
    cmake --build .
    cmake --install .
    ctest

CMake offers a large range of configuration options.
The most common options for building DLite are (with default values after the equal sign):
- **CMAKE_INSTALL_PREFIX**=\<path\>: Root directory for installing DLite.
  On Linux systems this defaults to `~/.local`.
- **WITH_PYTHON**=YES: Whether to build Python 3 bindings.
- **PYTHON_VERSION**=\<version\>: Python version to compile against.
  Example: `3.11`.
- **WITH_STATIC_PYTHON**=NO: Whether to compile with static python libraries.
- **WITH_FORTRAN**=NO: Whether to build Fortran bindings.
- **WITH_HDF5**=NO: Whether to build with HDF5 support.
- **WITH_REDLAND**=NO: Whether to build with RDF support (using librdf).
- **WITH_THREADS**=NO: Whether to build with threading support.
- **WITH_DOC**=NO: Whether to build documentation (sphinx+doxygen).
- **WITH_EXAMPLES**=NO: Whether to build/run examples during testing.
- **FORCE_EXAMPLES**=NO: Whether to force building/running examples.
- **ALLOW_WARNINGS**=NO: Whether to not fail on compilation warnings.

For example, to build against Python 3.10 with HDF5 support, configure with

    cmake -DWITH_PYTHON=YES -DPYTHON_VERSION=3.10 -DWITH_HDF5=YES ..

Please refer to the [CMake documentation] for more options.


### Build and install to a virtual Python environment
This is the most common case for a development installation on a Linux environment (including Ubuntu from WSL).

[virtualenvwrapper] is a set of convenient tools for working with virtual environments in Python.
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


### Build with Visual Studio
Please see the separate instructions for [building with Visual Studio](build_with_vs.md).


Dependencies
------------

### Build dependencies
For a basic build, at least the following is required:
- C compiler and standard C development libraries (ex: `sudo apt install build-essential`)
- [CMake] (ex `sudo apt install cmake`)

For configuring [CMake], the `ccmake` tool is handy.  Install it with `sudo apt install cmake-curses-gui`.  Run it as `ccmake .` from the build directory.

#### Python bindings (optional)
For Python bindings, you need Python with development libraries and [NumPy] installed.
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


### Runtime dependencies
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



[CMake]: https://cmake.org/
[CMake documentation]: https://cmake.org/cmake/help/latest/manual/cmake.1.html
[HDF5]: https://support.hdfgroup.org/HDF5/
[librdf]: https://librdf.org/
[Python 3]: https://www.python.org/
[NumPy]: https://pypi.org/project/numpy/
[virtualenvwrapper]: https://pypi.org/project/virtualenvwrapper/
[tripper]: https://pypi.org/project/tripper/
[PyYAML]: https://pypi.org/project/PyYAML/
[psycopg2]: https://pypi.org/project/psycopg2/
[pymongo]: https://github.com/mongodb/mongo-python-driver
[mongomock]: https://github.com/mongomock/mongomock
[pandas]: https://pandas.pydata.org/
