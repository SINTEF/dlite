Protocol plugins
================
Protocol plugins is a new feature of DLite that allow a clear separation between serialising/parsing DLite instances to/from external data representations and transferring data to/from external data resources.
This is illustrated in the Figure below.

![DLite storage and protocol plugins.](../../_static/storage-protocol.svg)

It allows to mix and match different protocol and storage plugins, thereby reducing the total number of plugins that has to be implemented.

For example can you combine a *bson* storage plugin with the *file*, *http* or *sftp* protocols to load an instance stored as BSON, from a file, a website or a SFTP server, respectively.

The `dlite.Instance` Python class, provides a simple interface to protocol and storage plugins via the  `dlite.Instance.load()` class method and the  `dlite.Instance.save()` method.


**Examples**

Accessing a YAML file using the *http* protocol:

```python
    >>> url = "https://raw.githubusercontent.com/SINTEF/dlite/refs/heads/master/storages/python/tests-python/input/test_meta.yaml"
    >>> dlite.Instance.load(protocol="http", driver="yaml", location=url)

```

Directly access to protocols
----------------------------
Protocol plugins can also be accessed in Python via the `Protocol` class.

This example first save the data `b"hello world"` to the file `hello.txt` and then read load it back again:
```python
    >>> with Protocol(protocol="file", location="hello.txt", options="mode=rw") as pr:
    ...     pr.save(b"hello world")
    ...     s = pr.load()
    >>> s
    b'hello world'

```

**Note** that all data are bytes objects.


Creating protocol plugins
-------------------------
Currently, DLite is shipped with the following protocols: *file*, *http*, *sftp*.
New protocols will be available if the path to the directory is added to the `dlite.python_protocol_plugin` path variable.

Please see the existing plugins, for how to implement a new plugin.
