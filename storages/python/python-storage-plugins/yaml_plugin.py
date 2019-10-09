import os

import yaml

import dlite
from dlite.options import Options

print("=== loading yaml_plugin...")


class yaml(DLiteStorageBase):
    """
    """
    #name = 'yaml'

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
        print("*** calling open(%r, %r)" % (uri, options))
        self.options = Options(options, default='mode=append')
        mode = dict(r='r', w='w', append='a')[self.options.mode]
        self.writable = False if mode == 'r' else True
        self.f = open(uri, mode)

    def close(self):
        """Closes this storage."""
        print('*** close()')
        self.f.close()

    def get_instance(self, uuid):
        """Loads `uuid` from current storage and return it as a new instance."""
        print('*** load')
        pass

    def set_instance(self, inst):
        """Stores `inst` in current storage."""
        print('*** write')
        if self.options.mode in ('w', 'append'):
            raise
            d = inst.todict()
            yaml.dump(d, self.f)
