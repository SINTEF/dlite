## Compile from source

The source code can be cloned from GitHub

```shell
git clone https://github.com/SINTEF/DLite.git
```

### Dependencies

#### Runtime dependencies

| **Name** | **Required / Optional** | **Notes** |
|:---:|:---:|:--- |
| [HDF5] | optional | Support v1.10+ (needed by HDF5 storage plugin). |
| [librdf] | optional | Needed by RDF (Redland) storage plugin. |
| [Python 3] | optional | Needed by Python bindings and some plugins. |
| [tripper] | required | For Python bindings. |
| [NumPy] | required | For Python bindings. |
| [PyYAML] | optional | Used for generic YAML storage plugin. |
| [psycopg2] | optional | Used for generic PostgreSQL storage plugin. Note that in some cases a GSSAPI error is raised when using psycopg2 by pip installing psycopg2-binary. This is solved by installing from source as described in their documentation. |
| [pandas] | optional | Used for CSV storage plugin. |
| [pymongo] | optional | Used for MongoDB storage plugin. |
| [mongomock] | optional | Used for testing MongoDB storage plugin. |

#### Build dependencies

| **Name** | **Required / Optional** | **Notes** |
|:---:|:---:|:--- |
| [cmake] | required | Note, cmake installed from PyPI does not always work. |
| [HDF5] | optional | Development libraries are needed by HDF5 storage plugin.</br>If ``-DWITH_HDF5=YES``. |
| [Python 3] | required | Development libraries are needed by Python bindings. |
| [NumPy] | required | Development libraries are needed by Python bindings. |
| [SWIG] | required | Needed for the Python bindings. |
| [Doxygen] | required | Used for documentation generation. |
| [Graphviz] | required | Used for documentation generation. |
| [valgrind] | optional | Used for memory checking (Linux only). |
| [cppcheck] | optional | Used for static code analysis. |
| [librdf] | optional | Development libraries are needed by RDF (Redland) storage plugin. |

## Compiling

### Build and install with Python

Given you have a C compiler and Python correctly installed, you should be able to build and install DLite via the python/setup.py script:

```shell
cd python
python setup.py install
```

### Build on Linux

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
make memcheck
```

`memcheck` runs all tests with memory checking (requires [valgrind]).

To generate code documentation, do

```shell
make Sphinx
```

You should be able to locate the documentation locally at build/pydoc/sphinx/index.html.

To install DLite locally, do

```shell
make install
```

### Build with Visual Studio on Windows

See [](build_with_vs.md) for detailed instructions for building with Visual Studio.

#### Quick start with Visual Studio Code and Docker

Using [Visual Studio Code] (VS Code) it is possible to do development on the system defined in [Dockerfile](https://github.com/SINTEF/dlite/blob/master/Dockerfile).

1. Download and install [Visual Studio Code].
2. Install the extension **Remote Development**.
3. [Clone _DLite_](#compile-from-source) and initialize git sub-modules:

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
   make && make test
   ```

### Build documentation

In order to reduce build dependencies for the causal user, DLite does not build documentation by default.
Provide the ``-DWITH_DOC=YES`` option to ``cmake`` to build the documentation.

#### Build Python Documentation

DLite uses sphinx to generate documentation from Python source code.
Ensure the correct virtual environment is set up and install the requirements

```shell
pip install -r requirements_doc.txt
```

#### Build C Documentation

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

[cmake]: https://cmake.org/
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
