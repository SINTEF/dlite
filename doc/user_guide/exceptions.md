Errors and exceptions
=====================
All errors in DLite are defined in the header file [src/dlite-errors.h].


Python exceptions
-----------------
If an error occur in the C-code of DLite, a DLite exception will be raised.
All error codes defined in [src/dlite-errors.h] are exposed as DLite exceptions in the Python interface and all DLite exceptions inherits from `dlite.DLiteError`.

DLite maintains its own error state, which is cleared before calling a function or method in the DLite Python API.
The only exception to this rule are the functions `dlite.errval()`, `dlite.errmsg()` and `dlite.errcheck()`.

`dlite.errval()` and `dlite.errmsg()` lets you investigate the latest error, without changing the error state.

`dlite.errcheck()` will re-raise (and clear) the latest error.

You can use `dlite.errclr()` to clear the error state.


Hiding errors
-------------
When an error occur in the C-code or  DLite, an error message starting with `** ` is printed to screen. For example:

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



### Catching a DLite exception
DLite maintains its own error state.  Therefore, if you catch a DLite exception, you should call `dlite.errclr()` when it is handled.  Otherwise the error will be re-raised the next time you call a DLite function.

For example:

```python
try:
    with dlite.Storage("mydriver", "somefile") as s:
        ...
except dlite.DLiteStorageOpenError:
    print("Skipping opening 'somefile'.  Cannot find driver: 'mydriver'")
    dlite.errclr()
```


C-level errors
--------------
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
