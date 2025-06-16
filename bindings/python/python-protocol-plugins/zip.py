"""DLite protocol plugin for zip files."""
from urllib.parse import urlparse

import hashlib
import re
import requests
import zipfile

import dlite
from dlite.options import Options
from dlite.utils import get_cachedir


class zip(dlite.DLiteProtocolBase):
    """DLite protocol plugin for zip archives."""

    def open(self, location, options=None):
        """Opens `location`.

        Arguments:
            location: A URL or path to a zip file.
                If `location` is a URL, a local cache is created and reused.
            options: The following options are supported:
                - timeout: Number of seconds before timing out when downloading
                  an URL.
                - nocache: If true and path is a URL, the zip file will be
                  downloaded again, even if a cache exists.
        """
        opts = Options(options, "timeout=1;nocache=no")
        nocache = dlite.asbool(opts["nocache"])
        path, zippath = location.split("#", 1)

        if re.match("^https?:.*", path):
            key = hashlib.shake_128(path.encode()).hexdigest(6)
            zipfile = get_cachedir() / f"cache-{key}.zip"
            if nocache or not self.zipfile.exists():
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
