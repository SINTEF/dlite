Example of MinIO for data and metadata store
============================================
In this example we use the free MinIO playground to store and fetch some data instances and metadata.

The script `store.py` will store data to MinIO.  Run it with `python store.py`.

The script `fetch.py` will fetch data from MinIO.  Run it with `python fetch.py`.
It also cleans up the storage afterwords.

The `main.py` script simply runs `store.py` and `fetch.py` in sequence.
It is intended to be used by the CI/CD framework.
