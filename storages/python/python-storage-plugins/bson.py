"""A DLite storage plugin for BSON written in Python."""
import os

import bson as pybson # Must be pymongo.bson

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict


class bson(dlite.DLiteStorageBase):
    """DLite storage plugin for BSON."""

    def open(self, uri, options=None):
        """Open `uri`.

        The `options` argument provies additional input to the driver.
        Which options that are supported varies between the plugins.
        It should be a valid URL query string of the form:

            key1=value1;key2=value2...

        An ampersand (&) may be used instead of the semicolon (;).

        Typical options supported by most drivers include:
        - mode : append (default) | r | w
            Valid values are:
            - append   Append to existing file or create new file
            - r        Open existing file for read-only
            - w        Truncate existing file or create new file

        After the options are passed, this method may set attribute
        `writable` to True if it is writable and to False otherwise.
        If `writable` is not set, it is assumed to be True.
        
        The BSON data is translated to JSON.
        """
        self.options = Options(options, defaults='mode=append')
        self.mode = dict(r='rb', w='wb', append='rb+')[self.options.mode]
        if self.mode == 'rb' and not os.path.exists(uri):
            raise FileNotFoundError(f"Did not find URI '{uri}'")
        self.writable = False if 'rb' in self.mode else True
        self.uri = uri
        self.d = {}
        if self.mode in ('rb', 'rb+'):
            with open(uri, self.mode) as f:
                bson_data = f.read()
            if pybson.is_valid(bson_data):
                self.d = pybson.decode(bson_data)
                if not self.d:
                    raise EOFError(f"Failed to read BSON data from '{uri}'")
            else:
                raise EOFError(f"Invalid BSON data in source '{uri}'")

    def close(self):
        """Close this storage and write the data to file.

        Assumes the data to store is in JSON format.
        """
        if self.writable:
            if self.mode == 'rb+' and not os.path.exists(self.uri):
                mode = 'wb'
            else:
                mode = self.mode
            for uuid in self.queue():
                props = self.d[uuid]['properties']
                if type(props) == dict: # Metadata props is list
                    for key in props.keys():
                        if type(props[key]) in (bytearray, bytes):
                            props[key] = props[key].hex()
                    self.d[uuid]['properties'] = props
            with open(self.uri, mode) as f:
                f.write(pybson.encode(self.d))

    def load(self, uuid):
        """Load `uuid` from current storage and return it
        as a new instance.
        """
        if uuid in self.d.keys():
            return instance_from_dict(self.d[uuid])
        else:
            raise KeyError(f"Instance with id '{uuid}' not found")

    def save(self, inst):
        """Store `inst` in the current storage."""
        self.d[inst.uuid] = inst.asdict()

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in
        the storage who's metadata URI matches global pattern
        `pattern`.
        """
        for uuid, d in self.d.items():
            if pattern and dlite.globmatch(pattern, d['meta']):
                continue
            yield uuid

