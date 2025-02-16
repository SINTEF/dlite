Debugging pytest
================
Small example with no dependencies on DLite that reproduces the pytest
segmentation fault error.

This segmentation fault appears when turning all pytest warnings into
errors with the following lines in pyproject.toml:

    [tool.pytest.ini_options]
    filterwarnings = ["error"]  # Treat all warnings as errors


Steps to reproduce the bug
--------------------------

1. Create a virtual environment and install dependencies

       python3 -m venv env
       source env/bin/activate
       pip install .

2. Build and install pytestdebug package

       cd pytestdebug  # Root directory of the pytestdebug example
       mkdir build
       cd build
       cmake ..
       make install

3. Run the test from the `tests` directory to avoid that pytest picks up
   the wrong `__init__.py` file

       cd ../tests
       pytest tests/test_pytestdebug.py

   This should give a segmentation fault.

4. Debugging with `gdb`

   ```gdb
   gdb python
   (gdb) r -m pytest test_pytestdebug.py
   (gdb) where
   (gdb) up
   (gdb) p swig_varlink_type()
   ```

   shows that `swig_varlink_type()` returns NULL, which is the course of the
   segfault. Running `gdb` again

   ```gdb
   gdb python
   (gdb) b swig_varlink_type
   (gdb) r -m pytest test_pytestdebug.py
   (gdb) n
   (gdb) n
   (gdb) p (PyTypeObject *)PyType_FromSpec(&spec)
   ```

   shows that the NULL comes from the Python library function `PyType_FromSpec()`
   returning NULL, although that the `spec` argument is fully legal.

Further debugging requires digging into the Python source code.
But from this it is seems very likely that pytest during its collection phase
calls the SWIG-generated interface before Python is properly initialised.


Conclusion
----------
Don't set `filterwarnings = ["error"]` in pytest.

Alternatively, select a more decent testing framework that doesn't loads modules
without importing them properly.
