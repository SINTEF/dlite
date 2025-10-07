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
[python-storage-plugins](python-storage-plugins/) directory.

See the [YAML plugin](python-storage-plugins/yaml_plugin.py) for a
simple example of a working storage plugin.



Plugins
=======

yaml
----
Generic plugin for reading and writing to file in YAML format.


bson
----
Generic plugin for reading and writing to file in BSON format.


pyrdf
-----
Generic plugin for reading and writing to file in RDF format.  It uses
rdflib. The plugin is called pyrdf to not confuse with the rdf plugin
implemented in C (using Redland librdf).


csv
---
Read and write to file in tabular CSV format.  The metadata for CSV
files should have only one dimension shared by all properties.


blob
----
Specialised plugin for reading and writing a binary blob to file.  The
content is specified using the metadata
http://onto-ns.com/meta/0.1/Blob (defined in the json file
[$DLITE_SOURCE_DIR/storages/python/python-storage-plugins/blob.json](https://github.com/SINTEF/dlite/blob/master/storages/python/python-storage-plugins/blob.json)).
It will be installed in the default metadata search path and seamless
accessible.


postgresql
----------
Generic plugin for reading and writing to a PostgreSQL database.

See https://docs.fedoraproject.org/en-US/quick-docs/postgresql/ for how to
install and setup a postgresql server on Fedora. Short instructions:

    sudo -u postgres psql
    postgres=# CREATE USER my_username WITH PASSWORD 'my_password';
    postgres=# CREATE DATABASE my_database OWNER my_username;



The `test_postgresql`_storage test require local configurations of the
PostgreSQL server.  The test is only enabled if a file pgconf.h can be
found in the [tests-c/](tests-c/) sub-directory.  Example pgconf.h file:

    #define HOST "pg_server_host"
    #define USER "my_username"
    #define DATABASE "my_database"
    #define PASSWORD "my_password"

Depending on how the server is set up, or if you have a ~/.pgpass
file, PASSWORD can be left undefined.

For running the tests, you may also have to run an ident server
running.  Install it with

    sudo dnf install oidentd

and start it with

    sudo systemctl start oidentd.service   # start now
    sudo systemctl enable oidentd.service  # start automatically when booting
