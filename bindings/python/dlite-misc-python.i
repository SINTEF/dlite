/* -*- python -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-misc.i */

%pythoncode %{
import dlite

class err():
    """Context manager for temporary turning off or redirecting errors.

    By default errors are skipped within the err context. But if
    ``filename`` is provided, the error messages are written to that file.
    Special file names includes:

    - ``None`` or empty: No output is written.
    - ``<stderr>``: Write errors to stderr (default).
    - ``<stdout>``: Write errors to stdout.

    """
    def __init__(self, filename=None):
        self.filename = filename

    def __enter__(self):
        self.f = dlite.err_get_stream()
        dlite.err_set_file(self.filename)
        return self.f

    def __exit__(self, *exc):
        dlite.errclr()
        dlite.err_set_stream(self.f)

silent = err()
%}
