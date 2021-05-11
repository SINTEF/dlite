"""A dumb storage plugin that simply reads/writes a file to/from an instance
as a binary blob.

The generated entity has no dimensions and one property called "content".
"""
import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict


class blob(DLiteStorageBase):
    """DLite storage plugin for binary blobs."""

    def open(self, uri, options=None):
        """Opens `uri`.

        Options
        -------
        id: string
            Explicit id of returned instance if reading.  Optional
        """
        self.options = Options(options, defaults='')
        self.uri = uri

    def close(self):
        """Closes this storage."""
        pass

    def load(self, uuid):
        """Loads `uuid` from current storage and return it as a new instance."""
        with open(self.uri, 'rb') as f:
            content = f.read()
        Meta = instance_from_dict({
            'meta': 'http://meta.sintef.no/0.3/EntitySchema',
            'uri': 'http://meta.sintef.no/0.1/Blob',
            'description': 'Entity representing a single binary blob.',
            'dimensions': [],
            'properties': [
                {
                    'name': 'content',
                    'type': 'blob%d' % len(content),
                    'description': 'Content of the binary blob.'
                }
            ]
        })
        print(Meta)
        inst = Meta(dims=(), id=self.options.get('id'))
        inst.content = content
        return inst

    def save(self, inst):
        """Stores `inst` in current storage."""
        with open(self.uri, 'wb') as f:
            f.write(inst.content)
