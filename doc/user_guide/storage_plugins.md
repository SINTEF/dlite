Storage plugins / Drivers
=========================

Content
-------
  1. [Introduction](#introduction)
  2. [How to make storage plugins available](#how-to-make-storage-plugins-available)
  3. [Using storages implicitely from Python](#using-storages-implicitly-from-python)
  4. [Working with storages in Python](#working-with-storages-in-python)
  5. [Writing Python storage plugins](#writing-python-storage-plugins)
  6. [Working with storages from C and Fortran](#working-with-storages-from-c-and-fortran)


Introduction
------------
A storage is in DLite an abstract concept that represent a generic data source or sink.
It can be a file on disk, a local database or a database accessed via a web interface.
Loading data from a storage into an instance and saving it back again is a key mechanism for interoperability at a syntactic level.

DLite provides a plugin system that makes it easy to connect to new data sources via a common interface (using a [strategy design pattern]).
Opening a storage takes in the general case four arguments, a `protocol` name identifying the [protocol plugin] to use for data transfer, a `driver` name identifying the storage plugin to use for parsing/serialisation, the `location` of the storage and a set of storage-specific `options`.

The `protocol` argument was introduced in v0.5.22 to provide a clear separation between transfer of raw data from/to the storage and parsing/serialisation of the data.

Storage plugins can be categorised as either *generic* or *specific*.
A generic storage plugin can store and retrieve any type of instance and metadata while a specific storage plugin typically deals with specific instances of one type of entity.
DLite comes with a set of generic storage plugins, like json, yaml, rdf, hdf5, postgresql and mongodb.
It also comes with a specific `Blob` and `Image` storage plugin, that can load and save instances of `http://onto-ns.com/meta/0.1/Blob` and `http://onto-ns.com/meta/0.1/Image`, respectively.
Storage plugins can be written in either C or Python.


How to make storage plugins available
-------------------------------------
As described below it is possible (and most often advisable) to create specific drivers (storage plugins) for your data.
Additional storage plugins drivers can be made available by setting the environment variables
`DLITE_STORAGE_PLUGIN_DIRS` or `DLITE_PYTHON_STORAGE_PLUGIN_DIRS` e.g.:
```bash
export DLITE_STORAGE_PLUGIN_DIRS=/path/to/new/folder:$DLITE_STORAGE_PLUGIN_DIRS
```

Within python, the path to the directory containing plugins can be added as follows:

```python
import dlite
dlite.python_storage_plugin_path.append("/path/to/plugins/dir")
```

Often drivers are connected to very specific datamodel (entities).
DLite will find these datamodels if the path to their directory is set with the
environment variable `DLITE_STORAGES` or added within python with `dlite.storage_path.append` similarly to described above for drivers.


```{attention}
Often, during development dlite will fail unexpectedly. This is typically either because of an error in the
datamodel or the driver.
The variable DLITE_PYDEBUG can be set as `export DLITE_PYDEBUG=` to get python debugging information.
This will give information about the driver.
It is advisable to first check that the datamodel is valid with the command `dlite-validate datamodelfilename`.
```

Using storages implicitly from Python
-------------------------------------
For convenience DLite also has an interface for creating storages implicitly.

### Loading an instance
If you only want to load a single instance from a storage, you can use one of the following class methods:
* `dlite.Instance.load()`: load from a location using a specific protocol
* `dlite.Instance.from_location()`: load from a location
* `dlite.Instance.from_url()`: load from URL
* `dlite.Instance.from_bytes()`: load from a buffer
* `dlite.Instance.from_dict()`: load from Python dict
* `dlite.Instance.from_json()`: load from a JSON string
* `dlite.Instance.from_bson()`: load from a BSON string
* `dlite.Instance.from_storage()`: load from a storage
* `dlite.Instance.from_metaid()`: create a new empty instance (not loading anything)

For example

```python
    >>> import dlite
    >>> newinst = dlite.Instance.from_location("json", "newfile.json", id="ex:blob1")
    >>> newinst.uri
    'ex:blob1'

```

To load a YAML file from a web location, you can combine the `http` [protocol plugin] with the `yaml` storage plugin using `dlite.Instance.load()`:

```python
    >>> url = "https://raw.githubusercontent.com/SINTEF/dlite/refs/heads/master/storages/python/tests-python/input/test_meta.yaml"
    >>> dlite.Instance.load(protocol="http", driver="yaml", location=url)

```

### Saving an instance
Similarly, if you want to save an instance, you can use the following methods:
* `dlite.Instance.save()`: save to location, optionally using a specific protocol.
  Can also take an open `dlite.Storage` object or an URL as argument.
* `dlite.Instance.to_bytes()`: returns the instance as a bytes object
* `dlite.Instance.asdict()`: returns the instance as a python dict
* `dlite.Instance.asjson()`: returns the instance as a JSON string
* `dlite.Instance.asbson()`: returns the instance as a BSON string

For example, saving `newinst` to  BSON, can be done with:

```python
    >>> newinst.save("bson", "newinst.bson", options="mode=w")

```


Working with storages in Python
-------------------------------
For instance, to create a new file-based JSON storage can in Python be done with:

```python
    >>> s = dlite.Storage("json", location="newfile.json", options="mode=w")

```

where the first argument is the name of the driver to use.
It is also possible to combine the three arguments into a URL as in

```python
    >>> s = dlite.Storage("json://newfile.json?mode=w")

```

Note that the URL form is less flexible, since the scheme part identifies the driver and can therefore not be used in the location of an external storage.

The options are plugin specific, but most file-based plugins take a `mode` option, with the following three values:
- `r`: Open existing file for read-only.
- `w`: Truncate existing file or create new file.
- `a`: Append to existing file or create new file.

The values of `driver`, `location` and `options` are available as attributes:

```python
    >>> s.driver
    'json'
    >>> s.location
    'newfile.json'
    >>> s.options
    'mode=w'

```

Currently, the Python interface does not implement a `close()` method to avoid risking unexpected crashes if one tries to access a closed storage object.
If you want to ensure a storage is closed (and possible buffered data is committed), you can delete it:

```python
    >>> del s

```

Alternatively, you can open a storage in a `with`-statement.
This is the preferred way, since it ensures that the storage is closed after use.

```python
    >>> with dlite.Storage("json", "newfile.json", options="mode=w") as s:
    ...     ...  # doctest: +SKIP

```

Instances can be stored using the `save()` method:


```python
    # Create some Blob data instances
    >>> Blob = dlite.get_instance('http://onto-ns.com/meta/0.1/Blob')
    >>> blob1 = Blob(dims={"n": 2}, id="ex:blob1")
    >>> blob2 = Blob(dims={"n": 5}, id="ex:blob2")

    # Add the instances to the storage
    # The option "mode=w" create a new JSON file (or overwrite it if it already exists)
    # The option "single=no" make sure that we store in a format that can accomodate multiple instances.
    >>> with dlite.Storage("json", location="newfile.json",
    ...                    options="mode=w;single=no") as s:
    ...     s.save(blob1)
    ...     s.save(blob2)
    ...     s.save(Blob)  # entities can also be saved

```

Note that we in the above example also store the Blob entity to the storage.
That can be a good idea if you don't have a proper metadata storage, since DLite needs the metadata in order to instantiate an instance.
When loading an instance and the metadata is not already in memory, DLite will first look for the metadata in the current and all other open storages before checking `dlite.storage_path`.


The instances can be loaded back from the storage with the `load()` method:

```python
    >>> with dlite.Storage("json", "newfile.json") as s:
    ...     inst1 = s.load(id="ex:blob1")

```

Note that we in the above example have omitted the `options` argument to `dlite.Storage`, since default is `mode=r`.
We also have to provide an `id` argument to `load()`, to specify which of the instances in the storage we want to load.
We can easily see that the loaded instance has the same UUID as the blob we stored:

```python
    >>> inst1.uuid == blob1.uuid
    True

```

In fact, even though `blob1` and `inst1` are different python objects, they share the same underlying memory:

```python
    >>> id(inst1) == id(blob1)
    False
    >>> blob1.content
    array([0, 0], dtype=uint8)
    >>> inst1.content = b'\x01\x08'
    >>> blob1.content
    array([1, 8], dtype=uint8)

```


Writing Python storage plugins
------------------------------
Storage plugins can be written in either C or Python.

In Python the storage plugin should be a Python module defining a subclass of `dlite.DLiteStorageBase` defining one or more of the following methods:
- **open()**: Open and initiate the storage.  Required if any of load(), save(), delete(), query(), flush(), close() are defined.
- **load()**: Load an instance from the storage and return it.
- **save()**: Save an instance to the storage.
- **delete()**: Delete and an instance from the storage.
- **query()**: Query the storage for instance UUIDs.
- **flush()**: Flushed cached data to the storage.
- **close()**: Close the storage.
- **from_bytes()**: Class method that loads an instance from a buffer.
- **to_bytes()**: Class method that saves an instance to a buffer.

A plugin can be defined with only the `from_bytes` and `to_bytes` methods. This has the advantage that only serialisation/deserialisation of the data and datamodels is considered in the actual plugin. In such cases Instances must
be instantiated with the dlite.Instance.load method, and the [protocol plugin] must be given.

All methods are optional. You only have to implement the methods providing the functionality you need.
See the [Python storage plugin template] for how to write a Python storage plugin.
A complete example can be found in the [Python storage plugin example].

In order for DLite to find the storage plugin, it should be in the search path defined by the `DLITE_PYTHON_STORAGE_PLUGIN_DIRS` environment variable or from Python, in `dlite.python_storage_plugin_path`.


:::{note}
**Prior to DLite v0.5.23 all storage plugins were executed in the same scope.**
This could lead to confusing and hard-to-find bugs due to interference between your plugins.
:::

However, since DLite v0.5.23, plugins are evaluated in separate scopes (which are available in `dlite._plugindict).



Working with storages from C and Fortran
----------------------------------------
The C API for storages is documented in the [C reference manual].
Conceptually it is similar to the Python interface, except that everything is implemented as functions.
For an example, see [ex1].

The Fortran interface relies heavily on code generation.
An example is available in [ex4].



[strategy design pattern]: https://en.wikipedia.org/wiki/Strategy_pattern
[C reference manual]: https://sintef.github.io/dlite/dlite/storage.html
[protocol plugin]: https://sintef.github.io/dlite/user_guide/storage_plugins.html
[Python storage plugin template]: https://github.com/SINTEF/dlite/blob/master/doc/user_guide/storage_plugin.py
[Python storage plugin example]: https://github.com/SINTEF/dlite/tree/master/examples/storage_plugin
[ex1]: https://github.com/SINTEF/dlite/tree/master/examples/ex1
[ex4]: https://github.com/SINTEF/dlite/tree/master/examples/ex4
