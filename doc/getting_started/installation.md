Installing DLite
================

Runtime dependencies
--------------------
DLite can be compiled without any external dependencies.
But you would normally install DLite compiled with Python support.
In that case, it has the following runtime dependencies:

  - [Python 3], required
    - [NumPy], required if Python is enabled
    - [tripper], optional, (used for property mappings)
    - [PyYAML], optional (used for generic YAML storage plugin)
    - [psycopg2], optional (used for generic PostgreSQL storage plugin)
        Note that in some cases a GSSAPI error is raised when using psycopg2
        by pip installing psycopg2-binary.
        This is solved by installing from source as described in their documentation.
    - [pandas], optional (used for csv storage plugin)
    - [pymongo], optional, (used for mongodb storage plugin)
    - [mongomock], optional, used for testing mongodb storage plugin.
  - [HDF5], optional, support v1.10+ (needed by HDF5 storage plugin)
  - [librdf], optional (needed by RDF (Redland) storage plugin)


Installing with pip
-------------------
If you are using Python, the easiest way to install DLite is with pip:

```shell
pip install DLite-Python
```


[HDF5]: https://support.hdfgroup.org/HDF5/
[librdf]: https://librdf.org/
[Python 3]: https://www.python.org/
[NumPy]: https://pypi.org/project/numpy/
[tripper]: https://pypi.org/project/tripper/
[PyYAML]: https://pypi.org/project/PyYAML/
[psycopg2]: https://pypi.org/project/psycopg2/
[pymongo]: https://github.com/mongodb/mongo-python-driver
[mongomock]: https://github.com/mongomock/mongomock
[pandas]: https://pandas.pydata.org/
