import json
import sys
from urllib.parse import quote_plus, urlparse

import pymongo

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict


class mongodb(dlite.DLiteStorageBase):
    """DLite storage plugin for PostgreSQL."""
    def open(self, uri, options=None):
        """Opens `uri`.

        The `options` argument provies additional input to the driver.
        Which options that are supported varies between the plugins.  It
        should be a valid URL query string of the form:

            key1=value1;key2=value2...

        An ampersand (&) may be used instead of the semicolon (;).

        Options:
        - database: Name of database to use (default: "dlite")
        - collection: Name of collection to use (default: "dlite_coll")
        - user: User name.
        - password: Password.
        - mode: "r" for opening in read-only mode, "w" for read/write mode.
        - authMechanism: Authentication mechanism (default: "SCRAM-SHA-256")
        - mock: Whether to use mongomock.
        """
        opts = Options(
            options,
            defaults='database=dlite;collection=dlite_coll;mode=w;'
            'authMechanism=SCRAM-SHA-256;mock=no',
        )
        opts.setdefault('password', None)
        self.writable = True if 'w' in opts.mode else False

        # Connect to existing database
        user = quote_plus(opts.user) if opts.user else None
        password = quote_plus(opts.password) if opts.password else None

        if dlite.asbool(opts.mock):
            import mongomock
            mongo_url = urlparse(f'mongodb://{uri}')
            port = mongo_url.port if mongo_url.port else 27017

            @mongomock.patch(servers=((mongo_url.hostname, port), ))
            def get_client():
                return open_client()
        else:
            def get_client():
                return open_client()

        def open_client():
            client = pymongo.MongoClient(
                host=uri,
                username=user,
                password=password,
                authSource=opts.database,
                authMechanism=opts.authMechanism,
            )
            return client

        self.client = get_client()
        self.collection = self.client[opts.database][opts.collection]
        self.options = opts

    def close(self):
        """Closes this storage."""
        self.client.close()

    def load(self, id):
        """Loads `id` from current storage and return it as a new instance."""
        uuid = dlite.get_uuid(id)
        document = self.collection.find_one({'uuid': uuid})
        return instance_from_dict(document)

    def save(self, inst):
        """Stores `inst` in current storage."""
        document = inst.asdict(uuid=True)
        self.collection.insert_one(document)

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who's metadata URI matches glob pattern `pattern`."""
        filter = {"meta": pattern} if pattern else {}
        for doc in self.collection.find(filter=filter, projection=["uuid"]):
            yield doc["uuid"]
