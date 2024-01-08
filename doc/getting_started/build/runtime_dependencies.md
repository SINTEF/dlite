Installing runtime dependencies
===============================
If DLite has been compiled with Python, Fortran, librdf or HDF5 support, you will need the corresponding runtime libraries.
These are normally installed together with the corresponding development libraries.
But if you are distributing DLite, the corresponding runtime libraries will have to be installed separately.
On Ubuntu, they can be installed with

    sudo apt install python3 gfortran librdf libhdf5

When DLite is compiled with Python bindings, additional runtime features may be enabled by installing one of more of the following optional Python packages
- [tripper], optional, (used for property mappings)
- [PyYAML], optional (used for generic YAML storage plugin)
- [psycopg2], optional (used for generic PostgreSQL storage plugin)
    Note that in some cases a GSSAPI error is raised when using psycopg2
    by pip installing psycopg2-binary.
    This is solved by installing from source as described in their documentation.
- [pandas], optional (used for csv storage plugin)
- [pymongo], optional, (used for mongodb storage plugin)
- [mongomock], optional, used for testing mongodb storage plugin.

These optional dependencies can be installed with

    pip install -r requirements_dev.txt


[tripper]: https://pypi.org/project/tripper/
[PyYAML]: https://pypi.org/project/PyYAML/
[psycopg2]: https://pypi.org/project/psycopg2/
[pandas]: https://pandas.pydata.org/
[pymongo]: https://github.com/mongodb/mongo-python-driver
[mongomock]: https://github.com/mongomock/mongomock
