/* -*- Python -*-  (not really, but good for syntax highlighting) */

/* Python-spesific extensions to dlite-behavior.i */


%extend _DLiteBehavior {
  %pythoncode %{
    def asdict(self):
        v = self.value
        return {
            "name": self.name,
            "value": None if v < 0 else True if v > 0 else False,
            "version_added": self.version_added,
            "version_new": self.version_new,
            "version_remove": self.version_remove,
            "description": self.description,
        }

    def __str__(self):
        import json
        return json.dumps(self.asdict(), indent=2)
  %}
}


%pythoncode %{


class _Behavior():
    """A class that provides easy access for setting and getting behavior
    settings.

    It is used via the singleton `dlite.Behavior`.

    The different behavior values can be accessed as attributes.

    """
    def __getattr__(self, name):
        v = self.get_record(name).value
        return None if v < 0 else True if v > 0 else False

    def __setattr__(self, name, value):
        if value is None:
            self.get_record(name).value = -1
        elif isinstance(value, bool):
            self.get_record(name).value = 1 if value else 0
        else:
            self.get_record(name).value = (
                -1 if value < 0 else 1 if value > 0 else 0
            )

    def __dir__(self):
        return object.__dir__(self) + list(self.get_names())

    def get_names(self):
        """Return a generator over all registered behavior names."""
        for i in range(_dlite._behavior_nrecords()):
            yield _dlite._behavior_recordno(i).name

    def get_record(self, name):
        """Return a record with information about the given behavior."""
        rec = _dlite._behavior_record(name)
        if not rec:
            raise DLiteKeyError(f"No such behavior record: {name}")
        return rec


# A singleton for accessing behavior settings.
Behavior = _Behavior()

%}
