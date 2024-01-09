"""DLite storage plugin for Redis written in Python."""
import re

from redis import Redis

import dlite
from dlite.options import Options


class redis(dlite.DLiteStorageBase):
    """DLite storage plugin for Redis."""

    def open(self, location: str, options=None):
        """Opens connection to a Redis server.

        Arguments:
            location: A fully resolved URI to the Redis database.
            options: Supported options:
            - `port`: Port to connect to (default: 6379).
            - `username`: Redis user name.
            - `password`: Redis password.
            - `socket_keepalive`: Whether to enable socket keepalive option.
              Seems to protect against hanging.  Default: true
            - `socket_timeout`: Timeout after given number of seconds.
            - `ssl`: Whether to ssl-encrypt the connection.
            - `ssl_certfile`: Path to SSL certificate signing request (.crt).
            - `ssl_keyfile`: Path to SSL private key file (.key).
            - `ssl_ca_certs`: Path to SSL certificate container (.pem).
            - `db`: Database number (default: 0).
            - `expire`: Number of seconds before new keys expire.
            - `only_new`: Wheter to only store instances that are not already
              in the storage.
            - `only_update`: Wheter to only update existing instances.
            - `fernet_key`: A base64-encoded 32-byte Fernet key.  If given,
              transparently encrypt all instances before sending them to Redis.
              Generate the key with `crystallography.fernet.generate_key()`.
        """
        opts = Options(
            options, defaults="port=6379;socket_keepalive=true;db=0"
        )

        # Pop out options passed to redis.set()
        self.setopts = {
            "nx": dlite.asbool(opts.pop("only_new", "false")),
            "xx": dlite.asbool(opts.pop("only_update", "false")),
        }
        if "expire" in opts:
            self.setopts["ex"] = int(opts.pop("expire"))

        self.fernet_key = opts.pop("fernet_key", None)

        # Fix option types and open Redis connection
        opts.port = int(opts.port)
        opts.socket_keepalive = dlite.asbool(opts.socket_keepalive)
        if "socket_timeout" in opts:
            opts.socket_timeout = int(opts.socket_timeout)
        if "ssl" in opts:
            opts.ssl = dlite.asbool(opts.ssl)
        opts.db = int(opts.db)
        self.redis = Redis(host=location, **opts)
        self.closed = False

    def close(self):
        """Close the connection to Redis."""
        self.redis.close()
        self.closed = True

    def load(self, id: str):
        """Loads `id` from current storage and return it as a new instance.

        Arguments:
            id: URI or UUID of DLite Instance to return.

        Returns:
            A DLite Instance corresponding to the given `id`.
        """
        uuid = dlite.get_uuid(id)
        data = self.redis.get(f"dlite:{uuid}")
        if data is None:
            raise dlite.DLiteError(f"No such instance redis storage: {uuid}")
        if self.fernet_key:
            from cryptography.fernet import Fernet

            f = Fernet(self.fernet_key.encode())
            data = f.decrypt(data)
        try:
            with dlite.errctl(hide=dlite.DLiteMissingMetadataError):
                return dlite.Instance.from_bson(data)
        except dlite.DLiteMissingMetadataError as exc:
            # If metadata cannot be found, load it from redis and try again
            match = re.match(f".*cannot find metadata '([^']*)'", str(exc))
            metaid, = match.groups()
            meta = self.load(metaid)
            return dlite.Instance.from_bson(data)

    def save(self, inst: dlite.Instance):
        """Stores `inst` in current storage.

        Arguments:
            inst: A DLite Instance to store in the storage.
        """
        data = bytes(inst.asbson())
        if self.fernet_key:
            from cryptography.fernet import Fernet

            key = self.fernet_key.encode()
            f = Fernet(self.fernet_key)
            data = f.encrypt(data)
        self.redis.set(f"dlite:{inst.uuid}", data, **self.setopts)

    def delete(self, id):
        """Delete instance with given `id` from storage.

        Arguments:
            id: URI or UUID of instance to delete.
        """
        uuid = dlite.get_uuid(id)
        self.redis.delete(f"dlite:{uuid}")

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who"s metadata URI matches glob pattern `pattern`.

        Arguments:
            pattern: A glob pattern to filter the yielded UUIDs.

        Yields:
            DLite Instance UUIDs based on `pattern`.
            If no `pattern` is given, all UUIDs are yielded from within the
            storage.

        Note:
            This method fetch all instances in the Redis store and may
            be slow on large databases.
        """
        for uuid_bytes in self.redis.keys("dlite:*"):
            uuid = uuid_bytes.decode()[6:]
            if pattern:
                # Consider to make pattern more efficient by storing
                # `uuid -> meta` mappings in redis as well...
                inst = self.load(uuid)
                if not dlite.globmatch(pattern, inst.meta.uri):
                    continue
            yield uuid
