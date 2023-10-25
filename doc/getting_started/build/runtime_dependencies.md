Installing runtime dependencies
===============================
If DLite has been compiled with Python, Fortran, librdf or HDF5 support, you will need the corresponding runtime libraries.
These are normally installed together with the corresponding development libraries.
But if you are distributing DLite, the corresponding runtime libraries will have to be installed separately.
On Ubuntu, they can be installed with

    sudo apt install python3 gfortran librdf libhdf5

When DLite is compiled with Python bindings, additional runtime features may be enabled by installing one of more of the following optional Python packages
- [tripper], used for property mappings
- [pint], used for units conversion in property mappings
- [pydantic], used for testing support for pydantic models
- [typing_extensions], needed by pydantic v2
- [rdflib], used by `pyrdf` storage plugin. 
- [PyYAML], used by `yaml` storage plugin
- [psycopg2], used by `postgresql` storage plugin
    Note that in some cases a GSSAPI error is raised when using psycopg2
    by pip installing psycopg2-binary.
    This is solved by installing from source as described in their documentation.
- [pandas], used by the `csv` storage plugin
- [requests], used by `http` storage plugin
- [jinja2], used by `template` storage plugin
- [pymongo], used by the `mongodb` storage plugin
- [redis], used by the `redis` storage plugin
- [minio], used by the `minio` storage plugin

These optional dependencies can be installed with

    pip install -r requirements_full.txt

Separate requirements can also be installed for just property mappings

    pip install -r requirements_mappings.txt

for development

    pip install -r requirements_dev.txt

or for building the documentation

    pip install -r requirements_doc.txt



[tripper]: https://pypi.org/project/tripper/
[pint]: https://pint.readthedocs.io/en/stable/
[pydantic]: https://docs.pydantic.dev/
[typing_extensions]: https://github.com/python/typing_extensions
[rdflib]: https://rdflib.readthedocs.io/
[PyYAML]: https://pypi.org/project/PyYAML/
[psycopg2]: https://pypi.org/project/psycopg2/
[pandas]: https://pandas.pydata.org/
[pymongo]: https://github.com/mongodb/mongo-python-driver
[mongomock]: https://github.com/mongomock/mongomock
[openpyxl]: https://openpyxl.readthedocs.io/
[requests]: https://requests.readthedocs.io/
[jinja2]: https://jinja.palletsprojects.com/
