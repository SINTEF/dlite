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
    cmake -DCMAKE_INSTALL_PREFIX=$WORKON_HOME/dlite39 -DPYTHON_VERSION=3.9 ..
    cmake --build .
    cmake --install .
    ctest

If the `-DPYTHON_VERSION=3.9` option to CMake is not sufficient to get CMake to compile against the correct libraries, you can make a copy of `cmake/build-python3.7.cmake` and configure CMake with

    cmake -C ../cmake/build-python3.7.cmake ..


:::{admonition} Note about VirtualEnvWrapper
:class: note,dropdown

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



[VirtualEnvWrapper]: https://virtualenvwrapper.readthedocs.io/en/latest/
