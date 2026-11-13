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
  Requires: psycopg

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


Running the storage tests that need a server
--------------------------------------------
The tests of the postgresql, redis, minio and mongodb storages require
running servers.  The easiest way to start them is with the
[compose.yaml](../../../test-services/compose.yaml) file in the
[test-services](../../../test-services) directory:

    docker compose -f test-services/compose.yaml up -d

Tests that need a server that is not running are skipped.  The
credentials are defined by `TEST_SERVICES` in
[testutils.py](../../../bindings/python/testutils.py) and can be
overridden with environment variables named
`DLITE_TEST_<SERVICE>_<KEY>`, e.g.

    export DLITE_TEST_POSTGRESQL_HOST=pg_server_host
    export DLITE_TEST_POSTGRESQL_PORT=5432
    export DLITE_TEST_POSTGRESQL_USER=my_username
    export DLITE_TEST_POSTGRESQL_PASSWORD=my_password
    export DLITE_TEST_POSTGRESQL_DATABASE=my_database



[format specification mini-language]: https://docs.python.org/3/library/string.html#formatspec
[Jinja]: https://jinja.palletsprojects.com/en/3.1.x/
