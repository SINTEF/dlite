Python storage plugins distributed with DLite
=============================================
This directory contains additional storage plugins written in Python,
including

* blob - a plugin for Binary Large OBjects (BLOBs).

* bson - a plugin for BSON (binary JSON). Requires the pymongo package.

* csv - a plugin for CSV files.

* postgresql - a PostgreSQL plugin that allows to serialise all types
  of dlite instances (including data instances, metadata, collections,
  etc) to a PostgreSQL database.  See below for how to enable the tests.

* yaml - a YAML plugin that is very similar to the json plugin
  implemented in C, except that it uses YAML instead of JSON.

  Including documentation, this plugin is only 72 lines of Python
  code.  Compare that to the more than 2800 codelines for the JSON
  plugin implemented in C.

Enabling the postgresql storage tests
-------------------------------------
The test_postgresql_storage test require local configurations of the
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
