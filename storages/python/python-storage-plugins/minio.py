"""DLite storage plugin for MinIO written in Python."""
from minio import Minio

import dlite
from dlite.options import Options


class minio(dlite.DLiteStorageBase):
    """DLite storage plugin for MinIO."""

    def open(self, uri: str, options=None):
        """Opens `uri`.

        Arguments:
            uri: Hostname of a S3 service.
            options: Supported options:
            - `bucket_name`: Name of bucket.  Defaults to "dlite".
            - `access_key`: Access key (aka user ID) of your account in S3 service.
            - `secret_key`: Secret Key (aka password) of your account in S3 service.
            - `session_token`: Session token of your account in S3 service.
            - `secure`: Whether to use secure (TLS) connection to S3 service or not.
            - `region`: Region name of buckets in S3 service.

        Currently unsupported Minio arguments:
            - `http_client`: Customized HTTP client.
            - `credentials`: Credentials provider of your account in S3 service.
        """
        self.uri = uri
        opts = Options(options, defaults="object_lock=false;secure=true")

        # Pop out bucket-related options
        self.bucket_name = opts.pop("bucket_name", "dlite")

        # Fix option types and create MinIO connection
        opts.secure = dlite.asbool(opts.secure)
        self.client = Minio(endpoint=uri, **opts)

        if not self.client.bucket_exists(bucket):
            kw = {"region": opts.region} if "region" in opts else {}
            self.client.make_bucket(self.bucket_name, **kw)

        self.closed = False

    def close(self):
        """Close the connection to Redis."""
        self.client.close()
        self.closed = True

    def load(self, id: str):
        """Loads `id` from current storage and return it as a new instance.

        Arguments:
            id: URI or UUID of DLite Instance to return.

        Returns:
            A DLite Instance corresponding to the given `id`.
        """
        uuid = dlite.get_uuid(id)
        try:
            response = self.client(
                bucket_name=self.bucket_name,
                object_name=uuid,
            )
            # Read data from response.
        finally:
            response.close()
            response.release_conn()

    def save(self, inst: dlite.Instance):
        """Stores `inst` in current storage.

        Arguments:
            inst: A DLite Instance to store in the storage.
        """
        data = bytes(inst.asbson())
        metadata = {"meta": inst.meta}
        if inst.uri:
            metadata["uri"] = inst.uri

        result = self.client.put_object(
            bucket_name=self.bucket,
            object_name=inst.uuid,
            data=io.BytesIO(data),
            length=len(data),
            content_type="application/bson",
            metadata=metadata,
        )
        # some checking of the result

    def delete(self, id):
        """Delete instance with given `id` from storage.

        Arguments:
            id: URI or UUID of instance to delete.
        """
        uuid = dlite.get_uuid(id)
        errors = self.client.remove_object(self.bucket_name, uuid)
        # TODO: check errors

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
        objects = self.client.list_objects(self.bucket_name, include_user_meta=True)
        for obj in objects:
            print(obj)
            #yield uuid
