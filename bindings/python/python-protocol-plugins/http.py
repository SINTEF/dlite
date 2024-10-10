"""DLite protocol plugin for http connections."""
from urllib.parse import urlparse

import requests

import dlite
from dlite.options import Options


class http(dlite.DLiteProtocolBase):
    """DLite protocol plugin for http and https connections."""

    def open(self, location, options=None):
        """Opens `location`.

        Arguments:
            location: A URL or path to a file or directory.
            options: Options will be passed as keyword arguments to
                requests.  For available options, see
                https://requests.readthedocs.io/en/latest/api/#requests.request
                A timeout of 1 second will be added by default.
        """
        self.location = location
        self.options = Options(options, "timeout=1")

    def load(self, uuid=None):
        """Return data loaded from file.

        If `location` is a directory, it is returned as a zip archive.
        """
        kw = {"params": uuid}
        kw.update(self.options)
        timeout = float(kw.pop("timeout"))
        r = requests.get(self.location, timeout=timeout, **kw)
        r.raise_for_status()
        return r.content

    def save(self, data, uuid=None):
        """Save `data` to file."""
        kw = {"params": uuid}
        kw.update(self.options)
        timeout = float(kw.pop("timeout"))
        r = requests.post(self.location, data=data, timeout=timeout, **kw)
        r.raise_for_status()

    def delete(self, uuid):
        """Delete instance with given `uuid`."""
        kw = {"params": uuid}
        kw.update(self.options)
        timeout = float(kw.pop("timeout"))
        r = requests.delete(self.location, timeout=timeout, **kw)
        r.raise_for_status()
