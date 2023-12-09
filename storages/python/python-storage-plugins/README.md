Python storage plugins distributed with DLite
=============================================
This directory contains additional storage plugins written in Python.


Generic plugins
---------------
Generic plugins supports all types of DLite instances (including data
instances, metadata, collections, etc).

- **bson**: Plugin for BSON (binary JSON). Requires the pymongo package.
  Requires: bson

- **csv**: Plugin for CSV and Excel files.
  Requires: pandas

- **http**: Plugin for fetching instances with HTTP GET requests.
  Requires: requests

- **minio**: Plugin for storing/retrieving instances from high-performance
  MinIO and Amazon S3 object storages.
  Requires: minio

- **mongodb**: Plugin for storing/retrieving instances from a MongoDB database.
  Requires: pymongo

- **postgresql**: A generic PostgreSQL plugin.  See below for how to enable the
  tests.
  Requires: psycopg2

- **pyrdf**: Plugin for serialising to/deserialising from RDF using rdflib.
  Requires: rdflib

- **redis**: Plugin for storing/retrieving instances from a Redis database.
  Requires: redis

- **template**: Plugin for serialising instances using a template. Both the
  built-in [format specification mini-language] and [Jinja] are supported.

- **yaml**: Plugin for serialising/deserialising YAML.  This is very similar to
  the built-in json plugin implemented in C, except that it uses YAML
  instead of JSON.
  Requires: PyYAML


Specialised plugins
-------------------
Specialised plugins supports storing/retrieving instances of a specific data model.

- **blob**: Plugin for storing blob (binary large object) instances described by
  the http://onto-ns.com/meta/0.1/Blob data model.

- **image**: Plugin for loading/saving images to/from instances of the
  http://onto-ns.com/meta/0.1/Image data model.  It supports additional operations
  like resizing, cropping, equalising the histogram, etc. when loading and/or saving
  an image.
  Requires: scikit-image


Enabling the postgresql storage tests
-------------------------------------
The test_postgresql_storage test requires local configurations of the
PostgreSQL server.  The tests are only enabled if a file pgconf.h can
be found in the [storage/python/tests](../tests) source directory.

Hence, to enable the postgresql storage tests, you should add the file
`storage/python/tests/pgconf.h` with the following content:

    #define HOST "pg_server_host"
    #define USER "my_username"
    #define DATABASE "my_database"
    #define PASSWORD "my_password"

Depending on how the server is set up, or if you have a ~/.pgpass
file, PASSWORD can be left undefined.



[format specification mini-language]: https://docs.python.org/3/library/string.html#formatspec
[Jinja]: https://jinja.palletsprojects.com/en/3.1.x/
