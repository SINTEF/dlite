"""DLite storage plugin for fetching instances with HTTP GET requests.

This simple plugin was initially added to support the entity service (see
https://github.com/SINTEF/dlite-entities-service), but may have other uses.
"""
import json
import requests

import dlite
from dlite.options import Options


class http(dlite.DLiteStorageBase):
    """DLite storage plugin that fetches instances with HTTP GET."""

    def open(self, location, options=None):
        """Opens `location`.

        Arguments:
            location: web address to access
            options: Supported options:
                - `single`: Whether the input is assumed to be in single-
                      entity form.  The default (`"auto"`) will try to infer
                      it automatically.
        """
        self.options = Options("single=auto")

        r = requests.get(location)
        self.content = json.loads(r.content)
        if "detail" in self.content:
            raise dlite.DLiteStorageOpenError(self.content["detail"])

    def load(self, id=None):
        """Returns instance retrieved from HTTP GET."""
        s = self.options.single
        single = s if s == "auto" else dlite.asbool(s)
        return dlite.Instance.from_dict(self.content, id=id, single=single)
