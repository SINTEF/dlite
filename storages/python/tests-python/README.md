# Python test suite for DLite storages

Just like the C tests in the folder **`dlite/storages/python/tests-c`**, the files in this folder run tests on the storage plugins in DLite, but through Python instead of C.

To run tests from the command line, run all of them with the main test script with
> **`python run_python_storage_tests.py`**
>
or run a single test with
> **`python [name of test].py`**
>

To add a new test, add a new Python script (use one of the existing ones as a template) and add the name of the new file to the **`tests`** list in **`run_python_storage_tests.py`**.