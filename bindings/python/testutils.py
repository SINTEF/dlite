"""Some utilities for testing."""
import dlite


class UnexpectedSuccessError(Exception):
    """Code that was expected to raise an exeption didn't do so."""


class UnexpectedExceptionError(Exception):
    """Code did not raise the expected exception."""


class raises():
    """Assert that a code block raises one of the expected exceptions
    listed in `exceptions`.

    Arguments:
        *exceptions: Expected exception classes.
        silent: Whether to silent DLite error messages in the code block.
    """
    def __init__(self, *exceptions, silent=True):
        self.exceptions = exceptions
        self.silent = silent

    def __enter__(self):
        if self.silent:
            # Silence DLite error messages from expected exceptions and save the
            # current err_ignore state
            self.save_silenced = {}
            for exc in self.exceptions:
                code = dlite._dlite._err_getcode(exc.__name__)
                self.save_silenced[code] = dlite._dlite._err_ignore_get(code)
                dlite._dlite._err_ignore_set(code, 1)

    def __exit__(self, exc_type, exc_value, tb):
        if self.silent:
            # Restore the err_ignore state
            for code, value in self.save_silenced.items():
                dlite._dlite._err_ignore_set(code, value)

        excnames = ", ".join(repr(exc.__name__) for exc in self.exceptions)
        if exc_type is None:
            raise UnexpectedSuccessError(
                f"Expected one of the following exceptions: {excnames}, "
                "but no exception was raised."
            )

        # Leave context manager gracefully if one of the expected exceptions
        # was raised
        for exc in self.exceptions:
            if issubclass(exc_type, exc):
                dlite.errclr()
                return True

        raise UnexpectedExceptionError(
            f"Expected one of the following exceptions: {excnames}, "
            f"but got: '{exc_type.__name__}'"
        ) from exc_value
