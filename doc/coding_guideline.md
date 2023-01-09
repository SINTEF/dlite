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
Tests should be added to


Style
-----
Follow the existing coding style.

Python code should ahead to [PEP8], and use [Google-style docstrings].



[PEP8]: https://peps.python.org/pep-0008/
[Google-style docstrings]: https://google.github.io/styleguide/pyguide.html#381-docstrings
