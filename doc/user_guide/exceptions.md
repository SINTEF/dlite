Errors and exceptions
=====================
All errors in DLite are defined in the header file [src/dlite-errors.h].


Reading error messages
----------------------
By default are error occurring in the C-code written to standard error as they happen.
An error happening deep inside in a nested function may be appended to by the calling functions.
For instance, if you try to read json input from an invalid path like "/", you may see the following error message printed to stderr:

> **Figure included for reviewer:** Shows the figure before merged to master. Should be removed...
>
![Error message example](https://github.com/SINTEF/dlite/blob/error-documentation/doc/_static/dlite-error.png)


![Error message example](https://github.com/SINTEF/dlite/blob/master/doc/_static/dlite-error.png)

_**Figure.** Example of a colourised nested error message printed to standard error._

The initial **DLiteOtherError** means that the inner nested function *cannot read from stream*. As a hint, it also ask you whether *it is a regular file with read permissions?*
The colon (:) followed by *Is a directory* is a system message that DLite appends.
The next line is appended by the calling function.
It tells that the file (or a directory in this case) that can't be read is "/".
The last line assigns the type **DLiteStorageLoadError** to the error and tells us that it is a JSON file that can't be loaded.


Python exceptions
-----------------
If an error occur in the C-code of DLite, a DLite exception will be raised.
All error codes defined in [src/dlite-errors.h] are exposed as DLite exceptions in the Python interface and all DLite exceptions inherits from `dlite.DLiteError`.

DLite maintains its own error state, which is cleared before calling a function or method in the DLite Python API.
The only exception to this rule are the functions `dlite.errval()`, `dlite.errmsg()` and `dlite.errcheck()`.

`dlite.errval()` and `dlite.errmsg()` lets you investigate the latest error, without changing the error state.

`dlite.errcheck()` will re-raise (and clear) the latest error.

You can use `dlite.errclr()` to clear the error state.


Controlling error messages
--------------------------

### Hiding error messages
Sometimes you don't want to show error messages, either from a specific code block or in general (e.g. to not clutter stderr).
A typical use case for is when you handle an error in a specific code block.
There are several ways errors can be hidden:

#### Hiding error messages with environment variables
General error messages can be ignored or redirected to file with the **[ERR_STREAM]** environment variable.
If empty, nothing will written, otherwise **[ERR_STREAM]** is a file name to which the error messages are appended.
The special values "stderr" (default) and "stdout" means write to standard error or standard output, respectively.

Whether to show warnings can be controlled with the **[ERR_WARN]** environment variable.


#### Hiding error messages from Python
In Python you can use `dlite.errctl()` to hide or redirect errors to a file.
`dlite.errctl()` can also be used as a context manager.
For example will:

```python
>>> with dlite.errctl(hide=dlite.DLiteStorageLoadError):
...     # Raises a dlite.DLiteStorageLoadError
...     dlite.Instance.from_location("json", "/")
```

hide the `DLiteStorageLoadError` that is raised when calling `Instance.from_location()` with an invalid path.


#### Hiding error messages in C
In C-code you can hide DLite error messages either by ignoring/redirecting them with `err_set_stream()` or with the `ErrTry` macro.
An `ErrTry` block for ignoring a `dliteValueError` may e.g. look like this:

```c
ErrTry:
 ... // code that may cause a dliteValueError...
ErrCatch(dliteValueError):
 break;  // ignoring dliteValueError
ErrEnd;
```

These error functions and macros can be found in the header file installed in `[include/dlite/utils/err.h]`.


### Colorised errors
By default errors are colorised (using [ANSI escape codes]) when printed to a terminal and not otherwise.
Uncolorised error messages are instead pre-pended with "** " to make them easier to spot.
Whether to colorise errors can be controlled with the **[ERR_COLOR]** environment variable.


### Debugging info
Additional debugging info, like source file and line number and function name can be added using the **[ERR_DEBUG]** environment variable.

To locate an error when debugging with e.g. gdb, it may sometimes be useful to call `abort()` when a DLite error is triggered.
This can be done by setting the **[ERR_ABORT]** environment variable to "abort".


Accessing errors at C-level
---------------------------
DLite functions normally indicate failures by returning NULL or a non-zero error code.
On failure, the error state can be accessed and cleared via the functions

```c
/* Return error code of last error. */
int dlite_errval(void);

/* Return error message of last error. */
const char *dlite_errmsg(void);

/* Clear the error indicator. */
void dlite_errclr(void);
```

which are defined in the header file installed in `[include/dlite/dlite-misc.h]`.



[ANSI escape codes]: https://en.wikipedia.org/wiki/ANSI_escape_code
[src/dlite-errors.h]: https://github.com/SINTEF/dlite/blob/master/src/dlite-errors.h
[include/dlite/utils/err.h]: https://github.com/SINTEF/dlite/blob/master/src/utils/err.h
[include/dlite/dlite-misc.h]: https://github.com/SINTEF/dlite/blob/master/src/dlite-misc.h
[ERR_STREAM]: https://github.com/SINTEF/dlite/blob/master/doc/user_guide/environment_variables.md
[ERR_WARN]: https://github.com/SINTEF/dlite/blob/master/doc/user_guide/environment_variables.md
[ERR_COLOR]: https://github.com/SINTEF/dlite/blob/master/doc/user_guide/environment_variables.md
[ERR_DEBUG]: https://github.com/SINTEF/dlite/blob/master/doc/user_guide/environment_variables.md
[ERR_ABORT]: https://github.com/SINTEF/dlite/blob/master/doc/user_guide/environment_variables.md
