Test services
=============

This directory contains a [compose.yaml](compose.yaml) file that starts
the external services used by the DLite tests.  It works with both
`docker compose` and `podman compose`.

| Service    | Port(s)    | Used by                                         |
|------------|------------|-------------------------------------------------|
| `postgres` | 5432       | postgresql storage (C and Python tests)         |
| `redis`    | 6379       | redis storage                                   |
| `minio`    | 9000, 9001 | minio storage (9001 is the web console)         |
| `mongodb`  | 27017      | mongodb storage (falls back to mongomock)       |
| `fuseki`   | 3030       | tripper tests (not used by DLite yet)           |
| `graphdb`  | 7200       | tripper tests (not used by DLite yet)           |


Usage
-----
Start all services from the root of the repository with

    docker compose -f test-services/compose.yaml up -d

or only some of them, e.g.

    docker compose -f test-services/compose.yaml up -d postgres redis

Add `--wait` to wait until the services are healthy.  Stop them again
with

    docker compose -f test-services/compose.yaml down

The compose project has the fixed name `dlite-test-services`, so the
services can also be managed with `docker compose -p dlite-test-services
...` from any directory.

Tests that need a service which is not running are skipped.

Note that the services publish their ports on localhost.  Stop any
locally running server that uses the same port first.


Credentials
-----------
The credentials are only meant for local testing.  They agree with the
defaults in `TEST_SERVICES` in
[bindings/python/testutils.py](../bindings/python/testutils.py) (and
[storages/python/tests-c/pgtestconf.h](../storages/python/tests-c/pgtestconf.h)).
They can be changed by setting environment variables named
`DLITE_TEST_<SERVICE>_<KEY>`, both when starting the services and when
running the tests, e.g.

    export DLITE_TEST_POSTGRESQL_PASSWORD=my-secret


Testing DLite and tripper together
----------------------------------
The fuseki and graphdb services are configured as expected by the
tripper tests, so the same services can be used when developing DLite
and tripper together.  Tripper only needs fuseki and graphdb:

    docker compose -f test-services/compose.yaml up -d fuseki graphdb

- fuseki is started with admin password `admin0` and a `test_repo`
  dataset.
- Starting graphdb also starts the one-shot `graphdb-init` service,
  which creates the `test_repo` repository with full-text search from
  [graphdb-test_repo-config.ttl](graphdb-test_repo-config.ttl), unless
  it already exists.


Continuous integration
----------------------
The [CI tests](../.github/workflows/ci_tests.yml) workflow starts the
services needed by the DLite tests from the same compose file.
