/* -*- Python -*-  (not really, but good for syntax highlighting) */

/* Python-specific extensions to dlite-behavior.i */


%extend _DLiteBehavior {
  %pythoncode %{
    def asdict(self):
        return {
            "name": self.name,
            "value": bool(self.value),
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
        return bool(_dlite._behavior_get(name))

    def __setattr__(self, name, value):
        _dlite._behavior_set(name, 1 if value else 0)

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
