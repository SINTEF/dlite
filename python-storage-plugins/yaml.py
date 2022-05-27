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

        Supported options:
        - mode : "a" | "r" | "w"
            Valid values are:
            - a   Append to existing file or create new file (default)
            - r   Open existing file for read-only
            - w   Truncate existing file or create new file
        - single : bool | "auto"
            Whether the YAML input is in single-entity form.  If `single` is
            "auto", it will be inferred from the input.

        After the options are passed, this method may set attribute
        `writable` to true if it is writable and to false otherwise.
        If `writable` is not set, it is assumed to be true.
        """
        self.options = Options(options, defaults='mode=append')
        self.mode = dict(r='r', w='w', append='r+')[self.options.mode]
        self.writable = False if 'r' in self.mode else True
        self.uri = uri
        self.d = {}
        if self.mode in ('r', 'r+'):
            with open(uri, self.mode) as f:
                d = pyyaml.safe_load(f)
            if d:
                self.d = d

    def close(self):
        """Closes this storage."""
        if self.writable:
            mode = ('w' if self.mode == 'r+' and not os.path.exists(self.uri)
                    else self.mode)
            with open(self.uri, mode) as f:
                pyyaml.dump(self.d, f, default_flow_style=False, sort_keys=False)

    def load(self, id):
        """Loads `uuid` from current storage and return it as a new instance."""
        uuid = dlite.get_uuid(id)
        #return instance_from_dict(self.d, id, single=self.options.single)
        return instance_from_dict(self.d, id)

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
