import fnmatch
import json
import sys
from urllib.parse import quote_plus, urlparse

import pymongo

import dlite
from dlite.options import Options


class mongodb(dlite.DLiteStorageBase):
    """DLite storage plugin for MongoDB."""

    def open(self, uri, options=None):
        """Opens `uri`.

        The `options` argument provies additional input to the driver.
        Which options that are supported varies between the plugins.  It
        should be a valid URL query string of the form:

            key1=value1;key2=value2...

        An ampersand (&) may be used instead of the semicolon (;).

        All options that begin with MONGOCLIENT_<keyword> will be
        passed directly to the mongodb client.

        Options:
        - database: Name of database to use (default: "dlite")
        - collection: Name of collection to use (default: "dlite_coll")
        - user: User name.
        - password: Password.
        - mode: "r" for opening in read-only mode, "w" for read/write mode.
        - schema: Schema to use when connecting to MongoDB server.  Defaults
              to schema in `uri`.
        - authMechanism: Authentication mechanism
        - mock: Whether to use mongomock.
        """
        parsed_options = self._parse_options(options)
        self._configure(parsed_options, uri)

    def _parse_options(self, options):
        """Parse and validate input options."""
        parsed_options = Options(
            options,
            defaults="database=dlite;collection=dlite_coll;mode=r;mock=no;"
            "user=guest;password=guest",
        )
        parsed_options.setdefault("password", None)
        return parsed_options

    def _configure(self, parsed_options, uri):
        """Configure and connect to the MongoDB database."""
        self.writable = True if "w" in parsed_options.mode else False

        client_options = {
            k: parsed_options[k] for k in parsed_options if "MONGOCLIENT_" in k
        }

        user = quote_plus(parsed_options.user) if parsed_options.user else None
        password = (
            quote_plus(parsed_options.password)
            if parsed_options.password
            else None
        )

        # Determine the schema based on the presence of "localhost" or "127.0.0.1" in the URI
        schema = parsed_options.get("schema", None)
        if schema is None:
            if "localhost" in uri or "127.0.0.1" in uri:
                schema = "mongodb"
            else:
                schema = "mongodb+srv"

        # Remove any existing schema from the URI
        if not uri.startswith(schema + "://"):
            uri = uri.replace("mongodb+srv://", "")
            uri = uri.replace("mongodb://", "")

        # Construct the final URI with the correct schema
        final_uri = f"{schema}://{uri}"

        if dlite.asbool(parsed_options.mock):
            import mongomock

            mongo_url = urlparse(f"mongodb://{uri}")
            port = mongo_url.port if mongo_url.port else 27017

            @mongomock.patch(servers=((mongo_url.hostname, port),))
            def get_client():
                return open_client()

        else:

            def get_client():
                return open_client()

        def open_client():
            client = pymongo.MongoClient(
                host=final_uri,
                username=user,
                password=password,
                **client_options,
            )
            return client

        self.client = get_client()
        self.collection = self.client[parsed_options.database][
            parsed_options.collection
        ]
        self.options = parsed_options

    def close(self):
        """Closes the MongoDB connection."""
        self.client.close()

    def load(self, id):
        """Loads `id` from current storage and return it as a new instance."""
        uuid = dlite.get_uuid(id)
        document = self.collection.find_one({"uuid": uuid})
        if not document:
            raise IOError(
                f"No instance with uuid={uuid} in MongoDB database "
                f'"{self.collection.database.name}" and collection '
                f'"{self.collection.name}"'
            )
        return dlite.Instance.from_dict(document, check_storages=False)

    def save(self, inst):
        """Stores `inst` in current storage."""
        document = inst.asdict(uuid=True)
        self.collection.insert_one(document)

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who's metadata URI matches glob pattern `pattern`."""
        # If a pattern is provided, convert it to a MongoDB-compatible
        # regular expression
        if pattern:
            # MongoDB supports PCRE, which is created by fnmatch.translate()
            mongo_regex = {"$regex": fnmatch.translate(pattern)}
            filter = {"meta": mongo_regex}
        else:
            filter = {}

        for doc in self.collection.find(filter=filter, projection=["uuid"]):
            yield doc["uuid"]
