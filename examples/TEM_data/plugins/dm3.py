"""DLite storage plugin for Gatan DM3 files."""
import sys
import json

import numpy as np
import ncempy.io as nio

import dlite


class dm3(dlite.DLiteStorageBase):
    """DLite storage plugin for Gatan DM3 files."""
    meta = "http://onto-ns.com/meta/characterisation/0.1/TEMImage"

    def open(self, location, options=None):
        """Loads a dm3 file from `location`.  No options are supported."""
        self.location = location

    def load(self, id=None):
        """Returns TEMImage instance."""

        # Load data and metadata
        d = nio.read(self.location)
        dm = nio.dm.fileDM(self.location)

        # Infer dimensions
        data = d["data"]
        shape = fill4(data.shape, 1)
        dimnames = "zSize2", "zSize", "ySize", "xSize"
        dimensions = dict([("ndim", 4)] + list(zip(dimnames, shape)))

        # Create and populate DLite instance
        TEMImage = dlite.get_instance(self.meta)
        temimage = TEMImage(dimensions=dimensions)
        temimage.filename = self.location
        temimage.data = np.array(data, ndmin=4)
        temimage.pixelUnit = fill4(d["pixelUnit"], "")
        temimage.pixelSize = fill4(d["pixelSize"], 0.0)
        temimage.metadata = json.dumps(dm.getMetadata(0), cls=Encoder)

        return temimage


class Encoder(json.JSONEncoder):
    """A json encoder that tackles numpy types."""
    def default(self, obj):
        if isinstance(obj, np.floating):
            return float(obj)
        if isinstance(obj, np.integer):
            return int(obj)
        if isinstance(obj, np.ndarray):
            return obj.tolist()
        return json.JSONEncoder.default(self, obj)


def fill4(seq, default=None):
    """Return sequence `seq` as a list of length 4.

    If `seq` is shorter than 4, it is padded from the start with the value of `default`
    """
    lst = [default]*4
    lst[4 - len(seq):] = seq
    return lst
