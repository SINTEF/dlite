Errors and exceptions
=====================
All errors in DLite are defined in the header file [src/dlite-errors.h].


Reading error messages
----------------------
By default are error occurring in the C-code written to standard error as they happen.
An error happening deep inside in a nested function may be appended to by the calling functions.
For instance, if you try to read json input from path "/", you may see the following error message printed to stderr:

```
DLiteOtherError: cannot read from stream. Is it a regular file with read permissions?: Is a directory
 - DLiteOtherError: error reading from file "/"
 - DLiteStorageLoadError: cannot load json file "/"
```

<code>
  <span class="ansi31">DLiteOtherError</span><span class="ansi2 ansi35">: cannot read from stream. Is it a regular file with read permissions?: Is a directory</span>
 - <span class="ansi31">DLiteOtherError</span><span class="ansi2 ansi35">: error reading from file "/"</span>
 - <span class="ansi31">DLiteStorageLoadError</span><span class="ansi2 ansi35">: cannot load json file "/"</span>
</code>

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


Controlling C-level errors
--------------------------

### Hiding error messages
When an error occur in the C-code or  DLite, an error message starting with `** ` is printed to standard error.
For example, if you call `dlite.Instance.from_location` with invalid *driver* (and *location*), you get:

```python
>>> dlite.Instance.from_location("", "")
** DLiteOtherError: missing driver
...
DLiteStorageOpenError: DLiteOtherError: missing driver

```

Sometimes these error messages are annoying.
To hide them, you can use the `dlite.errctl` context manager.
For example:

```python
>>> with dlite.errctl(hide=dlite.DLiteStorageOpenError):
...     dlite.Instance.from_location("", "")
DLiteStorageOpenError: DLiteOtherError: missing driver

```


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

defined in `src/dlite-mish.h`.



[src/dlite-errors.h]: https://github.com/SINTEF/dlite/blob/master/src/dlite-errors.h
