# Installing DLite

## Installing with pip

If you are using Python, the easiest way to install DLite is with pip:

```shell
pip install DLite-Python
```

Note, currently only Linux versions for Python 3.7, 3.8, 3.9 and 3.10 are available.
But Windows versions will soon be available.

## Docker image

A docker image is available on [GitHub](https://github.com/SINTEF/dlite/packages).

## Setting up the environment

As a DLite user it should be enough to do 'pip install Dlite-Python', or 'pip install .' from within the DLite/python directory.

As a developer it is more useful to install DLite from source.
If DLite is installed in a non-default location, you may need to set the PATH, LD_LIBRARY_PATH, PYTHONPATH and DLITE_ROOT environment variables.
See [](environment_variables.md) for more details.

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

Examples of how to use DLite are available in the [examples](https://github.com/SINTEF/dlite/tree/master/examples) directory.
