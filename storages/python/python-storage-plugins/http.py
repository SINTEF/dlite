import json
import requests

import dlite


class http(dlite.DLiteStorageBase):
    """DLite storage plugin that fetches entities with http GET."""

    def open(self, uri, options=None):
        """Opens `uri`."""

    def load(self, id):
        """Returns instance retrieved from HTTP GET on `id`."""
        r = requests.get(id)
        content = json.loads(r.content)
        if "detail" in content:
            raise dlite.DLiteError(content["detail"])
        return dlite.Instance.from_json(content)
