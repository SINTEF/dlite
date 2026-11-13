"""DLite storage plugin for fetching instances with HTTP GET requests.

This simple plugin was initially added to support the entity service (see
https://github.com/SINTEF/dlite-entities-service), but may have other uses.
"""
import os
import requests
import subprocess
import tempfile

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

        r = requests.get(location, timeout=3)
        if not r.ok:
            raise dlite.DLiteStorageOpenError(
                f"HTTP GET {location} failed: {r.status_code} {r.reason}"
            )
        self.content = r.json()
        if "detail" in self.content:
            raise dlite.DLiteStorageOpenError(self.content["detail"])

    def load(self, id=None):
        """Returns instance retrieved from HTTP GET."""
        s = self.options.single
        single = s if s == "auto" else dlite.asbool(s)
        return dlite.Instance.from_dict(self.content, id=id, single=single)

    # Note: untested method
    def save(self, inst):
        """Save instance to entity service.

        This requires that you have entities-service installed.

            pip install "git+https://github.com/SINTEF/entities-service.git#egg=project[cli]"

        """
        # We explicitly remove `tmpfile` in the finally statement to
        # avoid problems with file locking on Windows...
        with tempfile.NamedTemporaryFile(
            mode="wt", delete=False, suffix=".json"
        ) as f:
            tmpfile = f.name
        try:
            inst.save("json", tmpfile, "mode=w")
            subprocess.call(
                ["entities-service", "upload", "--file", tmpfile]
            )
        finally:
            os.remove(tmpfile)
