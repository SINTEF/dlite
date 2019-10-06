import os
import dlite

print("=== loading yaml_plugin...")


class yaml(DLiteStorageBase):
    """
    """
    name = None

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
        """
        pass

    def close(self):
        """Closes this storage."""
        pass

    def get_instance(self, uuid):
        """Loads `uuid` from current storage and return it as a new instance."""
        pass

    def set_instance(self, inst):
        """Stores `inst` in current storage."""
        pass
