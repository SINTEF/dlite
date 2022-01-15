The Python storage plugin
=========================
The Python storage plugin is a meta storage plugin that makes it
possible to load additional plugins written in Python.

In order to allow DLite to find your Python storage plugins, add the
directory with your plugins to the `DLITE_PYTHON_STORAGE_PLUGIN_DIRS`
environment variable.

Python storage plugins are implemented as Python classes subclassing
`DLiteStorageBase`, where `DLiteStorageBase` is a class defined in the
C code of the DLite Python plugin.  A storage plugin class must
implement the methods:

```python
    def open(self, uri, options=None):
        """Opens `uri`.

        The `options` argument provies additional input to the driver.
        Which options that are supported varies between the plugins.  It
        should be a valid URL query string of the form:

            key1=value1;key2=value2...

        An ampersand (&) may be used instead of the semicolon (;).

        Typical options supported by most drivers include:
        - mode : append | r | w
            Valid values are:
            - append   Append to existing file or create new file (default)
            - r        Open existing file for read-only
            - w        Truncate existing file or create new file

        After the options are passed, this method may set attribute
        `writable` to true if it is writable and to false otherwise.
        If `writable` is not set, it is assumed to be true.
        """

    def close(self):
        """Closes this storage."""
```

To provide functionality, it should also define one or more of the
following methods:

```python
    def load(self, uuid):
        """Loads `uuid` from current storage and return it as a new instance."""

    def save(self, inst):
        """Stores `inst` in current storage."""

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who's metadata URI matches glob pattern `pattern`."""
```

Python storage plugins provided with DLite can be found in the
[python_storage_plugins](python_storage_plugins/) directory.

See the [YAML plugin](python_storage_plugins/yaml_plugin.py) for a
simple example of a working storage plugin.
