"""DLite storage plugin for MinIO written in Python."""
import io

from minio import Minio

import dlite
from dlite.options import Options


class minio(dlite.DLiteStorageBase):
    """DLite storage plugin for MinIO."""

    def open(self, location: str, options=None):
        """Opens `uri`.

        Arguments:
            location: Hostname of a S3 service.
            options: Supported options:
            - `bucket_name`: Name of bucket.  Defaults to "dlite".
            - `access_key`: Access key (aka user ID) of your account in S3
               service.
            - `secret_key`: Secret Key (aka password) of your account in S3
               service.
            - `session_token`: Session token of your account in S3 service.
            - `secure`: Whether to use secure (TLS) connection to S3 service.
            - `region`: Region name of buckets in S3 service.

        Currently unsupported Minio arguments:
            - `http_client`: Customized HTTP client.
            - `credentials`: Credentials provider of your account in S3 service.
        """
        opts = Options(options, defaults="secure=true")

        # Pop out bucket-related options
        self.bucket_name = opts.pop("bucket_name", "dlite")

        # Fix option types and create MinIO connection
        opts.secure = dlite.asbool(opts.secure)
        self.client = Minio(endpoint=location, **opts)

        if not self.client.bucket_exists(self.bucket_name):
            kw = {"region": opts.region} if "region" in opts else {}
            self.client.make_bucket(self.bucket_name, **kw)

    def load(self, id: str):
        """Loads `id` from current storage and return it as a new instance.

        Arguments:
            id: URI or UUID of DLite Instance to return.

        Returns:
            A DLite Instance corresponding to the given `id`.
        """
        uuid = dlite.get_uuid(id)
        try:
            response = self.client.get_object(
                bucket_name=self.bucket_name,
                object_name=uuid,
            )
            return dlite.Instance.from_json(response.data.decode())
        finally:
            response.close()
            response.release_conn()

    def save(self, inst: dlite.Instance):
        """Stores `inst` in current storage.

        Arguments:
            inst: A DLite Instance to store in the storage.
        """
        # Consider to use the built-in BSON encoder for fast serialisation
        # of instances
        data = inst.asjson().encode()
        metadata = {"meta": inst.meta.uri}
        if inst.uri:
            metadata["uri"] = inst.uri

        self.client.put_object(
            bucket_name=self.bucket_name,
            object_name=inst.uuid,
            data=io.BytesIO(data),
            length=len(data),
            content_type="application/bson",
            metadata=metadata,
        )

    def delete(self, id):
        """Delete instance with given `id` from storage.

        Arguments:
            id: URI or UUID of instance to delete.
        """
        uuid = dlite.get_uuid(id)
        self.client.remove_object(self.bucket_name, uuid)

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who"s metadata URI matches glob pattern `pattern`.

        Arguments:
            pattern: A glob pattern to filter the yielded UUIDs.

        Yields:
            DLite Instance UUIDs based on `pattern`.
            If no `pattern` is given, all UUIDs are yielded from within the
            storage.
        """
        objects = self.client.list_objects(
            self.bucket_name, include_user_meta=True
        )
        for obj in objects:
            if pattern:
                print(obj.object_name, obj.metadata)
                if not dlite.globmatch(pattern, obj.metadata.get("meta")):
                    continue
            yield obj.object_name
