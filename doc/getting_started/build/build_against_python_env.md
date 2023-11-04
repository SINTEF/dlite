Build against Python environment
================================
This is the most common case for a development installation on a Linux environment (including Ubuntu from WSL).

[VirtualEnvWrapper] is a set of convenient tools for working with virtual environments in Python.
Install it with

    pip install --user virtualenvwrapper
    export WORKON_HOME=$HOME/.envs  # Put this in your ~/.bash_profile

Using [VirtualEnvWrapper], you can set up a new virtual environment for compile DLite against Python 3.9, with

    mkvirtualenv -p /usr/bin/python3.9 dlite39

The `-p` option to [VirtualEnvWrapper] allows you to select Python interpreter you want to build against.

Install Python dependencies to the new virtual environment

    workon dlite39
    pip install -U pip
    pip install -r requirements.txt
    pip install -r requirements_dev.txt

Create a new build folder and build DLite against Python 3.9

    workon dlite39
    mkdir build-dlite39
    cd build-dlite39
    cmake -DCMAKE_INSTALL_PREFIX=$VIRTUAL_ENV -DPython3_FIND_VIRTUALENV=ONLY -DPYTHON_VERSION=3.9 ..
    cmake --build .
    cmake --install .

If the `-DPYTHON_VERSION=3.9` option to CMake is not sufficient to get CMake to compile against the correct libraries, you can make a copy of `cmake/build-python3.7.cmake` and configure CMake with

    cmake -C ../cmake/build-python3.7.cmake ..

By default, [virtualenv] does not set `LD_LIBRARY_PATH`.
This will result in errors when running, for example, `dlite-codegen` in the example above.
To address this, DLite patches the virtualenv activate script to set `LD_LIBRARY_PATH` when running `cmake --install .` (see [patch activate] for details).

To update `LD_LIBRARY_PATH` in your current shell, please do

    deactivate
    workon dlite39

before running the tests

    ctest


[virtualenv]: https://docs.python.org/3/library/venv.html
[VirtualEnvWrapper]: https://virtualenvwrapper.readthedocs.io/en/latest/
[patch activate]: https://sintef.github.io/dlite/getting_started/build/patch_activate.html
