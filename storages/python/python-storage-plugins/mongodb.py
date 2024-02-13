
"""DLite storage plugin for MongoDB."""

import fnmatch
import json
import pymongo
import dlite
from dlite.options import Options


class mongodb(dlite.DLiteStorageBase):
    """DLite storage plugin for MongoDB."""

    def open(self, uri, options=None):
        """Opens `uri`, connection string of the MongoDB

        uri = mongodb://{account}:{password}@{account}.{hostname}:{port}/

        see more details of the connection string:
        https://www.mongodb.com/docs/manual/reference/connection-string/#connection-string-formats

        The `options` argument provies additional input to the driver.
        Which options that are supported varies between the plugins.  It
        should be a valid URL query string of the form:

            key1=value1;key2=value2...

        An ampersand (&) may be used instead of the semicolon (;).

        Options:
        - mode: "r" for opening in read-only mode, "w" for read/write mode.
        - database: Name of database to use (default: "test")
        - collection: Name of collection to use (default: "test_coll")
        - options for the constructor of the pymongo.MongoClient could pass by
          using the dlite.Options object, for example:

            opt = Options("mode=r;database=test;collection=testc")
            opt.update(directConnection=False, maxPoolSize=200)
            storage = dlite.Storage("mongodb", "localhost", str(opt))

        see more details of the pymongo.MongoClient options
        https://pymongo.readthedocs.io/en/stable/api/pymongo/mongo_client.html#pymongo.mongo_client.MongoClient:

        """
        opt = Options(
            options,
            defaults="mode=r;database=test;collection=test_coll"
        )
        keys = ["mode", "database", "collection"]
        client_options = {k: v for k, v in opt.items() if k not in keys}
        self.client = pymongo.MongoClient(uri, **client_options)
        self.collection = self.client[opt.database][opt.collection]
        self.options = opt
        self.readable = ("r" in opt.mode) | ("w" in opt.mode)
        self.writable = "w" in opt.mode

    def close(self):
        """Closes the MongoDB connection."""
        self.client.close()

    def load(self, id):
        """Loads `id` from current storage and return it as a new instance."""
        uuid = dlite.get_uuid(id)
        document = self.collection.find_one({"uuid": uuid})
        if not document:
            raise IOError(
                f"No instance with {uuid} in MongoDB database "
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
            filters = {"meta": mongo_regex}
        else:
            filters = {}
        for doc in self.collection.find(filter=filters, projection=["uuid"]):
            yield doc["uuid"]

    def delete(self, uid):
        """Delete instance with given `uid` from storage.

        Arguments:
            id: UUID of instance to delete.
        """
        result = self.collection.delete_one({"uuid": uid})
        return result.deleted_count == 1
