Build from source
=================

The source code can be cloned from GitHub

```shell
git clone https://github.com/SINTEF/DLite.git
```

Build dependencies
------------------
DLite uses [CMake] as its build system.
Depending on your selection of [CMake options], you would need to have the following tools and libraries installed on your system to build DLite:

  - [CMake], required for building - note that CMake isntalled from [PyPI] does not always work.
  - C compiler, required, ex. [gcc] or Microsoft [Visual Studio]
  - Underlying build tool, required, ex. [make] (selected with the CMake `-G` option)
  - [SWIG] optional, used for building Python bindings.
  - [Doxygen] optional, used for documentation generation.
  - [Graphviz] optional, used for documentation generation.
  - [valgrind], optional, used for memory checking (Linux only).
  - [cppcheck], optional, used for static code analysis.
  - [librdf] development libraries, optional, needed by librdf storage plugin.
  - [HDF5] development libraries, optional, needed by HDF5 storage plugin.
  - [Python 3] development libraries, optional, needed by Python bindings.

When DLite is build with Python support, you may also need to have the following Python packages installed:

  - [NumPy] development libraries, required by Python bindings.
  - [tripper], optional, used for property mappings.
  - [PyYAML], optional, used for generic YAML storage plugin.
  - [psycopg2], optional, used for generic PostgreSQL storage plugin. Note that in some cases a GSSAPI error is raised when using psycopg2 by pip installing psycopg2-binary. This is solved by installing from source as described in their documentation.
  - [pandas], optional, used for CSV storage plugin.
  - [pymongo], optional, used for MongoDB storage plugin.
  - [mongomock], optional, used for testing MongoDB storage plugin.


Build and install with Python
-----------------------------
Given you have a C compiler and Python correctly installed, you should be able to build and install DLite via the python/setup.py script:

```shell
cd python
python setup.py install
```

Build from virtual environment
------------------------------
As a DLite user it should be enough to do `pip install Dlite-Python`, or `pip install .` from within the `dlite/python` directory.

As a developer it is more useful to install DLite from source.
If DLite is installed in a non-default location, you may need to set the PATH, LD_LIBRARY_PATH, PYTHONPATH and DLITE_ROOT environment variables.
See [](../../user_guide/environment_variables.md) for more details.

An example of how to install DLite as developer within a python environment in linux is given below.
Make sure that all required dependencies are installed within the environment.

First activate the environment, e.g.:

```shell
source </path/to/dedicated/pythonenvironment>/bin/activate
```

Set the Python variables.
The following should automatically find the correct python paths

```shell
Python3_ROOT=$(python3 -c 'import sys; print(sys.exec_prefix)')
Python3_VERSION=$(python3 -c 'import sys;\
print(str(sys.version_info.major)+"."\
+str(sys.version_info.minor))')
Python3_EXECUTABLE=${Python3_ROOT}/bin/python${Python3_VERSION}
```

Python variables for developement libraries must be set **manually**.

```shell
Python3_LIBRARY=</path/to/system>/libpython${Python3_VERSION}.so
Python3_INCLUDE_DIR=</path/to/system>/include/python${Python3_VERSION}
```

To help find these paths, you may run

```shell
find . -name libpython*.so
```

Go into your DLite directory:

```shell
cd </path/to>/dlite
```

Build DLite:

```shell
mkdir build
cd build
cmake -DPython3_EXECUTABLE=$Python3_EXECUTABLE \
-DPython3_LIBRARY=$Python3_LIBRARY \
-DPython3_INCLUDE_DIR=$Python3_INCLUDE_DIR \
-DWITH_STATIC_PYTHON=FALSE \
-DCMAKE_INSTALL_PREFIX=$Python3_ROOT ..
```

Then install DLite

```shell
make && make install
```

Finally run tests

```shell
make test
```




Build on Linux
--------------
Install dependencies (e.g. with `apt-get install` on Ubuntu or `dnf install` on Fedora)

Configure the build with:

```shell
mkdir build
cd build
cmake ..
```

Configuration options can be added to the `cmake` command.
For example, you can change the installation directory by adding `-DCMAKE_INSTALL_PREFIX=/path/to/new/install/dir`.
The default is `~/.local`.

Alternatively, you can configure configuration options with `ccmake ..`.

If you use virtual environments for Python, you should activate your environment before running `cmake` and set `CMAKE_INSTALL_PREFIX` to the directory of the virtual environment.
For example:

```shell
VIRTUAL_ENV=/path/to/virtual/env
source $VIRTUAL_ENV/bin/activate
cmake -DCMAKE_INSTALL_PREFIX=$VIRTUAL_ENV -DWITH_DOC=YES ..
```

Build with:

```shell
make
```

To run the tests, do

```shell
ctest
```

To run all tests with memory checking (using [valgrind]), do

```
make memcheck
```

To generate code documentation (requires that you have passed `-DWITH_DOC=YES` to `cmake`), do

```shell
make Sphinx
```

You should be able to locate the documentation locally at `build/pydoc/sphinx/index.html`.

To install DLite locally, do

```shell
make install
```

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


Build with Visual Studio on Windows
-----------------------------------
See [](build_with_vs.md) for detailed instructions for building with Visual Studio.


Quick start with Visual Studio Code and Docker
----------------------------------------------
Using [Visual Studio Code] (VS Code) it is possible to do development on the system defined in [Dockerfile](https://github.com/SINTEF/dlite/blob/master/Dockerfile).

1. Download and install [Visual Studio Code].
2. Install the extension **Remote Development**.
3. [Clone _DLite_](#build-from-source) and initialize git sub-modules:

   ```shell
   git submodule update --init
   ```

4. Open the _DLite_ folder with VS Code.
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


Build documentation
-------------------
In order to reduce build dependencies for the causal user, DLite does not build documentation by default.
Provide the ``-DWITH_DOC=YES`` option to ``cmake`` to build the documentation.


### Build Python Documentation

DLite uses sphinx to generate documentation from Python source code.
Ensure the correct virtual environment is set up and install the requirements

```shell
pip install -r requirements_doc.txt
```

### Build C Documentation

If you have [Doxygen] installed, the HTML documentation should be generated as a part of the build process.
It can be browsed by opening the following file in your browser:

```shell
<build>/doc/html/index.html
```

where ``<build>`` is your build folder.

To only build the documentation, you can do:

```shell
cd build
cmake --build . --target doc
```

If you have LaTeX and ``make`` installed, you can also the latex documentation with

```shell
cd build
cmake --build . --target latex
```

which will produce the file

```shell
<build>/doc/latex/refman.pdf
```


[CMake]: https://cmake.org/
[CMake options]: cmake_options.md
[make]: https://www.gnu.org/software/make/
[Visual Studio]: https://visualstudio.microsoft.com/
[gcc]: https://gcc.gnu.org/
[PyPI]: https://pypi.org/
[cppcheck]: http://cppcheck.sourceforge.net/
[Doxygen]: http://www.doxygen.org/
[Graphviz]: https://www.graphviz.org/
[HDF5]: https://support.hdfgroup.org/HDF5/
[librdf]: https://librdf.org/
[mongomock]: https://github.com/mongomock/mongomock
[NumPy]: https://pypi.org/project/numpy/
[Python 3]: https://www.python.org/
[pandas]: https://pandas.pydata.org/
[psycopg2]: https://pypi.org/project/psycopg2/
[pymongo]: https://github.com/mongodb/mongo-python-driver
[PyYAML]: https://pypi.org/project/PyYAML/
[SWIG]: http://www.swig.org/
[tripper]: https://pypi.org/project/tripper/
[valgrind]: http://valgrind.org/
[Visual Studio Code]: https://code.visualstudio.com/
[vs-container]: https://code.visualstudio.com/docs/remote/containers#_quick-start-open-an-existing-folder-in-a-container
[VirtualEnvWrapper]: https://virtualenvwrapper.readthedocs.io/en/latest/
