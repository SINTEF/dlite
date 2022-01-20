"""A simple demonstrage of a DLite storage plugin written in Python."""
import os
import sys

import yaml as pyyaml

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict


class yaml(dlite.DLiteStorageBase):
    """DLite storage plugin for YAML."""

    def open(self, uri, options=None):
        """Opens `uri`.

        The `options` argument provies additional input to the driver.
        Which options that are supported varies between the plugins.  It
        should be a valid URL query string of the form:

            key1=value1;key2=value2...

        An ampersand (&) may be used instead of the semicolon (;).

        Typical options supported by most drivers include:
        - mode : append | r | w
            Valid values are:
            - append   Append to existing file or create new file (default)
            - r        Open existing file for read-only
            - w        Truncate existing file or create new file

        After the options are passed, this method may set attribute
        `writable` to true if it is writable and to false otherwise.
        If `writable` is not set, it is assumed to be true.
        """
        self.options = Options(options, defaults='mode=append')
        self.mode = dict(r='r', w='w', append='r+')[self.options.mode]
        self.writable = False if 'r' in self.mode else True
        self.uri = uri
        self.d = {}
        if self.mode in ('r', 'r+') and os.path.exists(uri):
            with open(uri, self.mode) as f:
                d = pyyaml.load(f, Loader=pyyaml.BaseLoader)
            if d:
                self.d = d

    def close(self):
        """Closes this storage."""
        if self.writable:
            mode = ('w' if self.mode == 'r+' and not os.path.exists(self.uri)
                    else self.mode)
            with open(self.uri, mode) as f:
                pyyaml.dump(self.d, f)

    def load(self, uuid):
        """Loads `uuid` from current storage and return it as a new instance."""
        uuid = dlite.get_uuid(uuid)
        return instance_from_dict(self.d[uuid])

    def save(self, inst):
        """Stores `inst` in current storage."""
        self.d[inst.uuid] = inst.asdict()

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who's metadata URI matches glob pattern `pattern`."""
        for uuid, d in self.d.items():
            if pattern and dlite.globmatch(pattern, d['meta']):
                continue
            yield uuid
