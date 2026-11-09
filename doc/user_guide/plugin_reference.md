Plugin reference
================

This page lists all plugins that ship with DLite, together with their
options and dependencies.  See [storage plugins] for how to use storage
plugins, [protocol plugins] for protocols and [mappings] for mapping
plugins.

Content
-------
  1. [Storage plugins (drivers)](#storage-plugins-drivers)
  2. [Protocol plugins](#protocol-plugins)
  3. [Availability of plugins](#availability-of-plugins)


Storage plugins (drivers)
------------------------

| Driver      | Language | Kind                | Requires            | Key options |
|-------------|----------|---------------------|---------------------|-------------|
| [json](#json)       | C    | generic | —            | `mode`, `single`, `uri-key`, `with-uuid`, `with-meta`, `arrays` |
| [hdf5](#hdf5)       | C    | generic | HDF5 (source build) | `mode` |
| [rdf](#rdf)         | C    | generic | Redland librdf (source build) | `mode`, `format` |
| [yaml](#yaml)       | Python | generic | PyYAML | `mode`, `soft7`, `single`, `with-uuid`, `with-meta`, `with_parent`, `uri-key` |
| [csv](#csv)         | Python | generic | pandas | `mode`, `meta`, `infer`, `format`, `pandas_opts`, `path`, `id` |
| [bson](#bson)       | Python | generic | pymongo | `mode`, `soft7` |
| [pyrdf](#pyrdf)     | Python | generic | rdflib | `mode`, `format` |
| [postgresql](#postgresql) | Python | generic | psycopg | `database`, `user`, `password`, `mode` |
| [mongodb](#mongodb) | Python | generic | pymongo | `mode`, `database`, `collection`, + `pymongo.MongoClient` options |
| [redis](#redis)     | Python | generic | redis | `port`, `username`, `password`, `ssl`, `db`, `expire`, … |
| [minio](#minio)     | Python | generic | minio | `bucket_name`, `access_key`, `secret_key`, `secure`, `region`, … |
| [http](#http)       | Python | generic | requests | `single` |
| [image](#image)     | Python | specific (`Image` entity) | scikit-image | — |
| [blob](#blob)       | Python | specific (`Blob` entity) | — | — |
| [template](#template) | Python | specific (template files) | jinja2 (optional) | `template`, `engine` |

All file-based plugins support the `mode` option with the values:
- `a`: Append to existing file or create new file (default).
- `r`: Open existing file for read-only.
- `w`: Truncate existing file or create new file.

### json

Serialises instances to/from JSON.  This is the built-in format used
when printing instances and by several of the `dlite.Instance`
convenience methods (`from_json()`, `asjson()`).

| Option | Description |
|--------|-------------|
| `mode` | `a`/`r`/`w` (see above). |
| `single` | Whether to write single-entity format.  The default (`auto`) infers it.  Use `single=no` to store multiple instances in one file. |
| `uri-key` | Whether to use the URI as JSON key (default: false). |
| `with-uuid` | Whether to include the UUID in output. |
| `with-meta` | Always include meta in output. |
| `arrays` | Serialise metadata dimensions and properties as arrays. |

Example — storing multiple instances in one file:

```python
>>> with dlite.Storage("json", "data.json", options="mode=w;single=no") as s:
...     s.save(inst1)
...     s.save(inst2)
```

### hdf5

Serialises instances to/from HDF5 files.  Only available in DLite
installations built from source with HDF5 enabled.

| Option | Description |
|--------|-------------|
| `mode` | `rw` (default), `r`, `w` or `a`. |

### rdf

Serialises instances to/from RDF, backed by the Redland librdf library.
Only available in DLite installations built from source against librdf.
For the RDF format when installing DLite from PyPI, use the Python-based
[pyrdf](#pyrdf) driver instead.

### yaml

Serialises instances to/from YAML.

| Option | Description |
|--------|-------------|
| `mode` | `a`/`r`/`w` (see above). |
| `soft7` | Whether to save using the SOFT7 format. |
| `single` | Whether to save in single-instance form. |
| `with-uuid` | Whether to include UUID when saving. |
| `with-meta` | Whether to always include "meta" (even for metadata). |
| `with_parent` | Whether to include parent info for transactions. |
| `uri-key` | Whether the URI is the preferred key in multi-instance format. |

The option names `with_uuid`, `with_meta` and `urikey` are deprecated
aliases for `with-uuid`, `with-meta` and `uri-key`, respectively
(see issue [#886]).

### csv

Reads/writes CSV (and other tabular formats supported by pandas, like
Excel when `openpyxl` is installed).  When loading, the metadata can be
inferred from the data source or provided explicitly.

| Option | Description |
|--------|-------------|
| `mode` | `r` (default) or `w`. |
| `meta` | URI to metadata describing the table to read.  Required if `infer` is false. |
| `infer` | Whether to infer metadata from the data source (default: true). |
| `path` | Additional search directories for `meta`. |
| `format` | Any format supported by pandas.  The default is inferred from the file extension. |
| `pandas_opts` | Comma-separated string of `key=value` options sent to pandas `read_<format>`/`to_<format>`.  String values should be quoted. |
| `id` | Explicit id of the returned instance when reading. |

Example — reading a CSV file with inferred metadata:

```python
>>> inst = dlite.Instance.from_location("csv", "data.csv", options="mode=r")
```

### bson

Serialises instances to/from BSON.  The BSON data is translated to JSON.

| Option | Description |
|--------|-------------|
| `mode` | `a`/`r`/`w` (see above). |
| `soft7` | Whether to save using the SOFT7 format. |

### pyrdf

Serialises instances to/from RDF, backed by the pure-Python [rdflib]
library.

| Option | Description |
|--------|-------------|
| `mode` | `a`/`r`/`w` (see above). |
| `format` | File format, e.g. `turtle`, `xml`, `n3`, `nt`, `json-ld`, `nquads`.  See the [rdflib documentation] for the complete list. |

### postgresql

Stores instances in a PostgreSQL database.

| Option | Description |
|--------|-------------|
| `database` | Name of database to connect to (default: `dlite`). |
| `user` | User name. |
| `password` | Password. |
| `mode` | `a`/`append` (default) or `r`. |

### mongodb

Stores instances in a MongoDB database.  The `location` is a MongoDB
connection string.

| Option | Description |
|--------|-------------|
| `mode` | `r` or `w`. |
| `database` | Name of database to use (default: `test`). |
| `collection` | Name of collection to use (default: `test_coll`). |

Additional options are passed to the constructor of `pymongo.MongoClient`.

### redis

Stores instances in a Redis database.

| Option | Description |
|--------|-------------|
| `port` | Port to connect to (default: 6379). |
| `username` | Redis user name. |
| `password` | Redis password. |
| `socket_keepalive` | Enable socket keepalive (default: true). |
| `socket_timeout` | Timeout in seconds. |
| `ssl` | Whether to SSL-encrypt the connection. |
| `ssl_certfile` | Path to SSL certificate file (`.crt`). |
| `ssl_keyfile` | Path to SSL private key file (`.key`). |
| `ssl_ca_certs` | Path to SSL certificate container (`.pem`). |
| `db` | Database number (default: 0). |
| `expire` | Number of seconds before new keys expire. |

### minio

Stores instances in a MinIO (S3-compatible) object store.  The `location`
is the hostname of the S3 service.

| Option | Description |
|--------|-------------|
| `bucket_name` | Name of bucket (default: `dlite`). |
| `access_key` | Access key (aka user ID) of your account. |
| `secret_key` | Secret key (aka password) of your account. |
| `session_token` | Session token of your account. |
| `secure` | Whether to use a secure (TLS) connection. |
| `region` | Region name of buckets. |
| `timeout` | Connection timeout in seconds. |

### http

Loads instances from a web address using [requests].  This is a
convenience driver; for full flexibility combine the `http` protocol
with any driver (see [protocol plugins]).

| Option | Description |
|--------|-------------|
| `single` | Whether the input is assumed to be in single-entity form.  The default (`auto`) infers it. |

### image

Specific plugin that loads/saves instances of the entity
`http://onto-ns.com/meta/0.1/Image`.  No options.  Requires
[scikit-image].

### blob

Specific plugin that loads/saves instances of the entity
`http://onto-ns.com/meta/0.1/Blob`.  No options.

### template

Specific plugin that renders an instance to a file using a template.
Useful for generating reports, input files, etc.  Requires a template
engine: the `format` method of the Python standard library (default) or
[jinja2].

| Option | Description |
|--------|-------------|
| `template` | Path to the template file. |
| `engine` | `format` (default) or `jinja`. |


Protocol plugins
----------------

Protocol plugins transfer raw bytes to/from external resources.  They
are combined with storage plugins via `dlite.Instance.load()` and
`dlite.Instance.save()` or used directly via the `Protocol` class
(imported from the `dlite.protocol` module).  See [protocol plugins]
for details and examples.

| Protocol | Description | Requires | Key options |
|----------|-------------|----------|-------------|
| [file](#file-protocol) | Read/write local files (and directories) | — | `mode`, `url` |
| [http](#http-protocol) | Download/upload via HTTP(S) | requests | passed to `requests.request` |
| [sftp](#sftp-protocol) | Transfer via SFTP | paramiko | `username`, `password`, `port`, `key_type`, … |
| [zip](#zip-protocol) | Read/write files inside zip archives (with local cache for URLs) | — | `timeout`, `nocache` |

### file protocol

Opens a local file or directory.

| Option | Description |
|--------|-------------|
| `mode` | Combination of `r` (read), `w` (write) or `a` (append).  Defaults to `r` if `location` exists and `w` otherwise. |
| `url` | Whether `location` is an URL.  By default this is inferred from whether it starts with a scheme. |

### http protocol

Downloads/loads data via HTTP(S).  All options are passed as keyword
arguments to [requests].  A timeout of 1 second is added by default.

### sftp protocol

Transfers data via SFTP.  The location may be just a host name or fully
qualified as `username:password@host:port`.

| Option | Description |
|--------|-------------|
| `username` | User name. |
| `password` | Password. |
| `hostname` | Host name. |
| `port` | Port number (default: 22). |
| `key_type` | Key type for key-based authorisation. |

### zip protocol

Reads/writes files inside zip archives.  If `location` is an URL, a
local cache is created and reused.

| Option | Description |
|--------|-------------|
| `timeout` | Number of seconds before timing out when downloading an URL. |
| `nocache` | If true and the location is an URL, download the zip file again even if a cache exists. |


Availability of plugins
-----------------------
Which plugins are available depends on how DLite was installed:

- Installing DLite with `pip install dlite-python` gives a minimal
  installation, where the only built-in storage plugins are `json`
  (and `bson`/`rdf` when compiled against the corresponding libraries).
- Installing the optional requirements (see `requirements_full.txt` in
  the [DLite repository]) enables the full set of Python plugins
  listed above: `yaml`, `csv`, `postgresql`, `mongodb`, `redis`, `minio`,
  `http`, `image`, `pyrdf`, `template`, `blob`, …
- The C plugins `hdf5` and `rdf` are only available in installations
  built from source with HDF5/librdf enabled.

:::{note}
The `single`/`with-uuid`-style options of file-based drivers influence
the layout of the produced files.  When in doubt, refer to the driver's
docstring, which is the authoritative documentation:

```python
>>> import dlite
>>> help(dlite.Storage("json", "data.json", "mode=r"))
```
:::


[storage plugins]: https://sintef.github.io/dlite/user_guide/storage_plugins.html
[protocol plugins]: https://sintef.github.io/dlite/user_guide/protocol_plugins.html
[mappings]: https://sintef.github.io/dlite/user_guide/mappings.html
[requests]: https://requests.readthedocs.io/
[rdflib documentation]: https://rdflib.readthedocs.io/en/stable/intro_to_parsing.html
[rdflib]: https://rdflib.readthedocs.io/
[scikit-image]: https://scikit-image.org/
[jinja2]: https://jinja.palletsprojects.com/
[DLite repository]: https://github.com/SINTEF/dlite
[#886]: https://github.com/SINTEF/dlite/issues/886
