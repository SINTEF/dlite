"""DLite protocol plugin for http connections."""
from urllib.parse import urlparse

import hashlib
import re
import requests
import zipfile

import dlite
from dlite.options import Options
from dlite.utils import get_cachedir


class zip(dlite.DLiteProtocolBase):
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
        opts = Options(options, "timeout=1")
        path, zippath = location.split("#", 1)

        if re.match("^https?:.*", path):
            key = hashlib.shake_128(path.encode()).hexdigest(6)
            zipfile = get_cachedir() / f"cache-{key}.zip"
            if not self.zipfile.exists():
                r = requests.get(path, timeout=float(opts["timeout"]))
                r.raise_for_status()
                with open(self.zipfile, "wb") as f:
                    f.write(r.content)
        else:
            zipfile = path

        self.zipfile = zipfile  # zip archieve
        self.zippath = zippath  # path within the zip archieve
        self.options = opts

    def load(self, uuid=None):
        """Return data loaded from file within a zip archive."""
        with zipfile.ZipFile(self.zipfile, mode="r") as fzip:
            with fzip.open(self.zippath, mode="r") as f:
                return f.read()
