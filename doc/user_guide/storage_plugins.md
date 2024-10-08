Storage plugins
===============

Content
-------
  1. [Introduction](#introduction)
  2. [Working with storages in Python](#working-with-storages-in-python)
  3. [Using storages implicitely](#using-storages-implicitly)
  4. [Writing Python storage plugins](#writing-python-storage-plugins)
  5. [Working with storages from C and Fortran](#working-with-storages-from-c-and-fortran)


Introduction
------------
A storage is in DLite an abstract concept that represent a generic data source or sink.
It can be a file on disk, a local database or a database accessed via a web interface.
Loading data from a storage into an instance and saving it back again is a key mechanism for interoperability at a syntactic level.

DLite provides a plugin system that makes it easy to connect to new data sources via a common interface (using a [strategy design pattern]).
Opening a storage takes three arguments, a `driver` name identifying the storage plugin to use, the `location` of the storage and `options`.

Storage plugins can be categorised as either *generic* or *specific*.
A generic storage plugin can store and retrieve any type of instance and metadata while a specific storage plugin typically deals with specific instances of one type of entity.
DLite comes with a set of generic storage plugins, like json, yaml, rdf, hdf5, postgresql and mongodb.
It also comes with a specific Blob storage plugin, that can load and save instances of a `http://onto-ns.com/meta/0.1/Blob` entity.
Storage plugins can be written in either C or Python.


Working with storages in Python
-------------------------------
For instance, to create a new file-based JSON storage can in Python be done with:

```python
    >>> import dlite
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


Using storages implicitly
-------------------------
For convenience DLite also has an interface for creating storages implicitly.
If you only want to load a single instance from a storage, you can use one of the following class methods:
* `dlite.Instance.from_location()`: to read from a location
* `dlite.Instance.from_url()`: to read from URL

For example

```python
    >>> newinst = dlite.Instance.from_location(
    ...    "json", "newfile.json", id="ex:blob1")
    >>> newinst.uri
    'ex:blob1'

```

DLite instances also have the methods `save()` and `save_to_url()` for saving to a storage without first creating a `dlite.Storage` object.

Saving this instance to BSON, can be done in a one-liner:

```python
    >>> newinst.save("bson", "newinst.bson", options="mode=w")

```


Writing Python storage plugins
------------------------------
Storage plugins can be written in either C or Python.

In Python the storage plugin should be a Python module defining a subclass of `dlite.DLiteStorageBase` with a set of methods for opening, closing, reading, writing and searching the storage.
In order for DLite to find the storage plugin, it should be in the search path defined by the `DLITE_PYTHON_STORAGE_PLUGIN_DIRS` environment variable or from Python, in `dlite.python_storage_plugin_path`.

See the [Python storage plugin template] for how to write a Python storage plugin.
A complete example can be found in the [Python storage plugin example].


:::{danger}
**For DLite <0.5.23 storage plugins were executed in the same scope.
Hence, to avoid confusing and hard-to-find bugs due to interference between your plugins, you should not define any variables or functions outside the `DLiteStorageBase` subclass!**
:::

Since DLite v0.5.23, plugins are evaluated in separate scopes (which are available in `dlite._plugindict).




Working with storages from C and Fortran
----------------------------------------
The C API for storages is documented in the [C reference manual].
Conceptually it is similar to the Python interface, except that everything is implemented as functions.
For an example, see [ex1].

The Fortran interface relies heavily on code generation.
An example is available in [ex4].



[strategy design pattern]: https://en.wikipedia.org/wiki/Strategy_pattern
[C reference manual]: https://sintef.github.io/dlite/dlite/storage.html
[Python storage plugin template]: https://github.com/SINTEF/dlite/blob/master/doc/user_guide/storage_plugin.py
[Python storage plugin example]: https://github.com/SINTEF/dlite/tree/master/examples/storage_plugin
[ex1]: https://github.com/SINTEF/dlite/tree/master/examples/ex1
[ex4]: https://github.com/SINTEF/dlite/tree/master/examples/ex4
