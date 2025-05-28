"""DLite storage plugin for serialising TEMImage metadata to json."""
import json

import dlite


class temsettings(dlite.DLiteStorageBase):
    """DLite storage plugin for serialising TEMImage metadata to json.

    Arguments:
        location: Path to output JSON file.
        options: Unused
    """
    meta = "http://onto-ns.com/meta/characterisation/0.1/TEMImage"

    def open(self, location, options=None):
        """Open output file `location`.  No options are supported."""
        self.location = location

    def save(self, inst):
        """Stores TEMImage metadata to storage."""
        with open(self.location, mode="w") as f:
            settings = json.loads(inst.metadata)
            json.dump(settings, f, indent=2)
