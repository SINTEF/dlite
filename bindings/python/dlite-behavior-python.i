/* -*- Python -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-behavior.i */

%pythoncode %{


class Behavior():
    """A class that provides easy access for setting and getting behavior
    settings.
    """
    def __getattr__(self, name):
        return self.get_record(name).value

    def __setattr__(self, name, value):
        self.get_record(name).value = value

    def __dir__(self):
        return object.__dir__(self) + list(self.get_names())

    def get_names(self):
        """Return a generator over all registered behavior names."""
        for i in range(_dlite._behavior_nrecords):
            yield _dlite._behavior_recordno(i).name

    def get_record(self, name):
        """Return a record with information about the given behavior."""
        rec = _dlite._behavior_record(name)
        if not rec:
            raise DLiteKeyError(f"No such behavior record: {name}")
        return rec

%}
