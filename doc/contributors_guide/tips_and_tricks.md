Tips and Tricks
===============

Setting up a virtual Python environment for building dlite
----------------------------------------------------------
See [Build against Python environment] in the installation instructions.


Test installation before releasing on PyPI
------------------------------------------
If you have updated the installation of the [dlite-python] package, or you get failures on GitHub CI/CD that cannot be reproduced locally, you might want to test installing dlite-python before releasing it on PyPI.

This can be done as follows:

1. Create a new virtual environment and install requirements and the wheel package

       python -m venv ~/.envs/testenv
       source ~/.envs/testenv/bin/activate
       pip install -U pip

2. Build the wheel

       cd dlite  # root of source directory
       pip install wheel
       pip wheel -w python/dist ./python

3. Install the wheel with pip in a newly created environment (the version numbers may differ for your case)

       pip install python/dist/DLite_Python-0.5.27-cp311-cp311-linux_x86_64.whl

4. Finally, test by importing dlite in the standard manner

       python
       >>> import dlite

   or you can run the python tests

       python bindings/python/tests/test_python_bindings.py


Debugging Python storage plugins
--------------------------------
Exceptions occurring inside Python storage plugins are not propagated to the calling interpreter, and will therefor not be shown.
However, it is possible to write them to stderr by setting the `DLITE_PYDEBUG` environment variable.

For example, run:

    DLITE_PYDEBUG= python bindings/python/tests/test_storage.py 1>/dev/null

to see tracebacks of exceptions occurring inside any of the tested storage plugins.


Debugging tests failing inside docker on GitHub
-----------------------------------------------
1. Set up a local virtual environment with the same version of Python as in the failing test.
   See [Build against Python environment].
2. Enter your virtual environment with dlite and install cibuildwheel.

        workon dlite
        pip install cibuildwheel

3. To list all manylinux images for Python 3.12, do

        cd dlite  # Root of DLite source directory
        CIBW_MANYLINUX_X86_64_IMAGE=ghcr.io/sintef/dlite-python-manylinux_2_34_x86_64:latest \
        CIBW_BUILD=cp312-manylinux_* \
        python -m cibuildwheel \
        --print-build-identifiers \
        --platform linux \
        python

   This should write

        cp312-manylinux_x86_64
        cp312-manylinux_i686

4. Run image.  For example, to run the image `cp312-manylinux_x86_64` do

        CIBW_MANYLINUX_X86_64_IMAGE=ghcr.io/sintef/dlite-python-manylinux_2_34_x86_64:latest \
        CIBW_BUILD=cp312-manylinux_x86* \
        python -m cibuildwheel \
        --output-dir wheelhouse \
        --platform linux \
        python

   which should run the tests and hopefully fail at the same place as on
   GitHub.

   If that is the case, you can run the image again, but pause
   it before running the tests by prepending `CIBW_BEFORE_TEST=cat` to
   the previous command:

        CIBW_BEFORE_TEST=cat \
        CIBW_MANYLINUX_X86_64_IMAGE=ghcr.io/sintef/dlite-python-manylinux_2_34_x86_64:latest \
        CIBW_BUILD=cp312-manylinux_x86* \
        python -m cibuildwheel \
        --output-dir wheelhouse \
        --platform linux \
        python


5. Now you can list all active containers with

        docker ps

    the first container should have a name starting with `cibuildwheel-`.
    Copy its id and run

        ID=91312b22090f  # replace this id with the one listed by docker ps -a
        docker exec -it $ID /bin/sh

    This should start a new shell inside the container that the test was run it.


6. Activate the virtual environment that was used to build DLite

        source /tmp/tmp.*/venv/bin/activate

   and install final requirements manually

        cd /project/python/build
        pip install virtualenv -c /constraints.txt
        pip install --cache-dir /ci/pip_cache --prefer-binary -r /project/requirements.txt

7. Locate and install the DLite_Python wheel

        ls /tmp/cibuildwheel/repaired_wheel/DLite_Python-*.whl

        pip install /tmp/cibuildwheel/repaired_wheel/DLite_Python-*.whl

8. Now we can run the Python tests with

        python /project/bindings/python/tests/test_python_bindings.py


### Docker network
If `pip wheel` fails with network errors, like `[Errno 101] Network is unreachable`, you may have to create a network

     docker network create -d bridge dlitenet  # Create network called dlitenet

and get its IP with

     ip=$(docker network inspect dlitenet | sed -n 's/ *"Gateway": "\(.*\)"$/\1/p')

You can then rerun `cibuildwheel` with the

     CIBW_BUILD_FRONTEND="pip; args: --index-url http://$ip:3141/root/pypi/" \
     CIBW_MANYLINUX_X86_64_IMAGE=ghcr.io/sintef/dlite-python-manylinux_2_34_x86_64:latest \
     CIBW_BUILD=cp312-manylinux_x86* \
     python -m cibuildwheel \
     --output-dir wheelhouse \
     --platform linux \
     python



Memory debugging
----------------
On Linux systems, one can use [valgrind] to check for and debug memory issues.


### Finding memory issues
To check for any memory issue, you can run valgrind on all tests with

    make memcheck

However, this usually takes a while. If you have localised an issue, and want
to rerun valgrind on that issue, you can run

    CTEST_ARGS="-R <test_name>" make memcheck

Both of the above commands will write the output to `<BUILD_DIR>/Testing/Temporary/MemoryChecker.<#>.log`, where `<#>` is the test number.

```note
The first time you run `make memcheck`, a supression file will be created, suppressing issues that does not comes from DLite.
Generating a suppression file may take long time, but it will only be generated once unless you manually add more suppressions to one of the `cmake/valgrind-*.supp` files.
```

---

Alternatively, you can run valgrind manually, with

    valgrind <path/to/test-executable>

if the test is written in C, or with

    valgrind python <path/to/test_file.py>

if the test is written in Python.
This will write the output from valgrind to stderr.


### Debugging segmentation faults
If a test results in a segmentation fault, you can debug it with [gdb].
See the [GDB Tutorial] for how to use `gdb`.

Tests written in C can be debugged with

    $ gdb <path/to/test-executable>
    (gdb) run

Test written in Python can be debugged with

    $ gdb python
    (gdb) run <path/to/test_file.py>

Some useful gdb commands:
- `where`: show the call stack when the test fails.
- `up`, `down`: navigated up and down the call stack.
- `print <expression>`: print the value of a variable or expression.


### Debugging memory issues
To debug other memory issues, invoke `gdb` as shown above, but set a breakpoint
at the function before issuing the `run` command.  For example:

    $ gdb python
    (gdb) break <c_function_name>
    (gdb) run <path/to/test_file.py>

More useful gdb commands:
- `break <function_name>`: set a break point at given function.
- `break <file:lineno>`: set a break point at given source file position.
- `continue`: continue to run the program after a break point.
- `next`: execute next source line.
- `step`: steps into the outermost subroutine on the next source line.
  Behaves like `next` if the next source line has no subroutines.


[virtualenvwrapper]: https://pypi.org/project/virtualenvwrapper/
[Build against Python environment]: https://sintef.github.io/dlite/getting_started/build/build_against_python_env.html#build-against-python-environment
[dlite-python]: https://pypi.org/project/DLite-Python/
[valgrind]: http://valgrind.org/
[gdb]: https://sourceware.org/gdb/
[GDB Tutorial]: https://www.gdbtutorial.com/
