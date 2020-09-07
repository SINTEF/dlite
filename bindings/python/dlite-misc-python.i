/* -*- c -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-misc.i */

%pythoncode %{
class err():
    """Context manager for temporary turning off or redirecting errors.

    By default errors are skipped within the err context.  But if
    `filename` is provided, the error messages are written to that file.
    Special file names includes
      - None or empty: no output is written
      - <stderr>: write errors to stderr (default)
      - <stdout>: write errors to stdout
    """
    def __init__(self, filename=None):
        self.filename = filename

    def __enter__(self):
        self.f = err_get_stream()
        err_set_file(self.filename)
        return self.f

    def __exit__(self, *exc):
        errclr()
        err_set_stream(self.f)
%}
