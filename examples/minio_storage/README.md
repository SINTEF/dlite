Example of MinIO for data and metadata store
============================================
In this example we use the free MinIO playground to store and fetch some data instances and metadata.
This storage can be accessed from DLite using the following URL: `minio://play.min.io?access_key=Q3AM3UQ867SPQQA43P2F;secret_key=zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG`.
The `access_key` and `secret_key` should normally be kept private, but MinIO has kindly made the corresponding account publicly available for testing purposes.
Hence, it should be possible for everyone to run this example.

MinIO is well suited for storing both data and corresponding data models (metadata).
Normally, you would use separate buckets for data and metadata.
But for simplicity, we will in this example just use the default bucket (called "dlite") for both the data and metadata.

The data we will use are chemical compositions two common aluminium alloys, [aa6060] and [aa6082], described by the [http://onto-ns.com/meta/calm/0.1/Chemistry] entity.


Storing
-------
First we will store our data instances to MinIO.
That is done in the script [store.py].
Since we have several instances, the script use `dlite.Storage` to reduce the number of connections made to MinIO:

```python
with dlite.Storage(url) as s:
    s.save(inst1)  # Do this for each instance
    s.save(inst1.meta)  # Remember to also store the datamodel
    ...
```
where `url` is the URL for the MinIO playground shown above.

You can test this script by running:

    python store.py


Fetching
--------
To fetch data from MinIO, we will first append the URL to the MinIO playground to the dlite storage search path.
That can either be done by appending to the `DLITE_STORAGES` environment variable (note that this variable uses the pipe (|) character to separate URLs) or the `dlite.storage_path` object in Python.
The latter is done in the [fetch.py] script.
After the storage path has been set, you can simply access data and metadata by UUID or UUID using `dlite.get_instance()`.

You can test this script by running:

    python fetch.py


main.py
-------
The `main.py` script simply runs `store.py` and `fetch.py` in sequence.
It is intended to be used by the CI/CD framework.



[aa6060]: ../entities/aa6060.json
[aa6082]: ../entities/aa6082.json
[http://onto-ns.com/meta/calm/0.1/Chemistry]: ../entities/Chemistry-0.1.json
[store.py]: store.py
[fetch.py]: fetch.py
