Coding guideline
================
These are guidelines for contributing code to DLite.


Error handling
--------------
DLite should be a robust code base.
All cases that can result in an error should be reported.

From C are errors is the error status normally reported via the return value.
Error messages are reported with the `err()` function or the `FAILCODE` family of macros.
Use error codes from `src/dlite-errors.h` when reporting an error.
Think an extra time before adding a new error code to this file.

From Python standard exceptions are used.

It is recommended to use `assert` in both C and Python to check for cases that should never happen.
If they due to a bug happen anyway, you rather want an early and clean abort, than the code to fail in undefined ways, perhaps even in completely unrelated places.


Testing
-------
New features should be tested.
All tests are run with [ctest].

Testing of C code is currently done with the minimal [MinUnit] C/C++ testing framework.
For Python, we use the most suited testing framework for the test at hand.
Standalone test scripts are well suited for simple cases, because they are easy to understand and allows efficient interactive development and debugging.
For more complex cases, needing test separation or fixtures, full-blown [pytest] should be used.


Style
-----
Follow the existing coding style.

Python code should ahead to [PEP8], and use [Google-style docstrings].



[PEP8]: https://peps.python.org/pep-0008/
[Google-style docstrings]: https://google.github.io/styleguide/pyguide.html#381-docstrings
[ctest]: https://cmake.org/cmake/help/latest/manual/ctest.1.html
[MinUnit]: https://github.com/siu/minunit/
[pytest]: https://docs.pytest.org/
