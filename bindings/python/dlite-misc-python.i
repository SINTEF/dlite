/* -*- python -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-misc.i */

%pythoncode %{

import atexit

atexit.register(_mark_python_atexit)


class errctl():
    """Context manager for temporary disabling specific DLite error
    messages or redirecting them.

    Arguments:
        hide: Sequence of DLiteException subclasses or exception names
            corresponding to error messages to hide.  A single class or
            name is also allowed.
            May also be a boolean, in which case error messages will be
            hidden/shown.
        show: Sequence of DLiteException subclasses or exception names
            corresponding to error messages to show.  A single class or
            name is also allowed.
            May also be a boolean, in which case error messages will be
            shown/hidden.
        filename: Filename to redirect errors to.  The following values
            are handled specially:
              - "None" or empty: No output is written.
              - "<stderr>": Write errors to stderr (default).
              - "<stdout>": Write errors to stdout.

    """
    def __init__(self, hide=(), show=(), filename="<stderr>"):
        allcodes = [-i for i in range(1, _dlite._get_number_of_errors())]

        if hide is True or show is False:
            self.hide = allcodes
        elif hide is not False:
            self.hide = self._as_codes(hide)

        if show is True or hide is False:
            self.show = allcodes
        elif show is not False:
            self.show = self._as_codes(show)

        self.filename = filename

    def __enter__(self):
        self.mask = _dlite._err_mask_get()
        for code in self.hide:
            _dlite._err_ignore_set(code, 1)
        for code in self.show:
            _dlite._err_ignore_set(code, 0)
        self.f = _dlite.err_get_stream()
        _dlite.err_set_file(self.filename)

    def __exit__(self, *exc):
        ignored = _dlite._err_ignore_get(_dlite.errval())
        _dlite._err_mask_set(self.mask)
        _dlite.err_set_stream(self.f)
        if ignored or self.filename is None:
            _dlite.errclr()

    @staticmethod
    def _as_codes(seq):
        """Return sequence of exceptions/exception names as a sequence of
        DLite error coces.  `seq` may also be a single exception or name."""
        sequence = [seq] if isinstance(seq, (str, type)) else seq
        errnames = [
            exc.__name__ if isinstance(exc, type) else str(exc)
            for exc in sequence
        ]
        if "DLiteError" in errnames:
            return [-i for i in range(1, _dlite._get_number_of_errors())]
        return [_dlite._err_getcode(errname) for errname in errnames]


silent = errctl(filename="None")
%}
