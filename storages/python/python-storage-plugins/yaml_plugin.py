import os
import sys

import yaml as pyyaml

import dlite
from dlite.options import Options
from dlite.utils import instance_from_dict


class yaml(DLiteStorageBase):
    """
    """

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
        mode = dict(r='r', w='w', append='w+')[self.options.mode]
        self.writable = False if mode == 'r' else True
        self.f = open(uri, mode)

    def close(self):
        """Closes this storage."""
        self.f.close()

    def load(self, uuid):
        """Loads `uuid` from current storage and return it as a new instance."""
        uuid = dlite.get_uuid(uuid)
        d = pyyaml.load(self.f)
        inst = instance_from_dict(d[uuid])
        print('--- loaded inst', id(inst))
        print(inst)
        print()
        return inst

    def save(self, inst):
        """Stores `inst` in current storage."""
        print('*** storing inst:', inst.uuid)
        d = {}
        if self.options.mode == 'append':
            d = pyyaml.load(self.f)
        d[inst.uuid] = inst.asdict()
        pyyaml.dump(d, self.f)
