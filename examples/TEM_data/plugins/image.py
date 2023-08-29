"""DLite storage plugin for images."""
import numpy as np

from skimage.io import imread, imsave
from skimage.exposure import equalize_hist
from skimage.transform import resize

import dlite
from dlite.options import Options


class image(dlite.DLiteStorageBase):
    """DLite storage plugin for images.

    Arguments:
        location: Path to YAML file.
        options: Supported options:
            - `plugin`: Name of plugin to use.
            - `as_gray`: Whether to convert color images to gray-scale
              when loading.  Default is false.
            - `equalize`: Whether to equalize histogram before saving.
              Default is false.
            - `resize`: Required image size when saving.  Should be given
              as `HEIGHTxWIDTH` (ex: "256x128").
    """
    meta = "http://onto-ns.com/meta/0.1/Image"

    def open(self, location, options=None):
        """Loads an image from `location`.  No options are supported."""
        self.location = location
        self.options = Options(
            options, defaults="as_gray=false;equalize=false"
        )

    def load(self, id=None):
        """Returns TEMImage instance."""
        as_gray = dlite.asbool(self.options.as_gray)
        data = imread(
            self.location, as_gray=as_gray, plugin=self.options.get("plugin"),
        )

        # Infer dimensions
        shape = [1]*3
        shape[3 - len(data.shape):] = data.shape
        dimnames = "channels", "height", "width"
        dimensions = dict(zip(dimnames, shape))

        # Create and populate DLite instance
        Image = dlite.get_instance(self.meta)
        image = Image(dimensions=dimensions)
        image.filename = self.location
        image.data = np.array(data, ndmin=3)

        return image

    def save(self, inst):
        """Stores DLite instance `inst` to storage."""
        if dlite.asbool(self.options.equalize):
            data = equalize_hist(inst.data)
        else:
            data = inst.data

        if data.shape[0] == 1:
            data = data[0,:,:]

        if self.options.get("resize"):
            size = [int(s) for s in self.options.resize.split("x", 1)]
            shape = list(data.shape)
            shape[data.ndim-2:] = size
            data = resize(data, output_shape=shape, order=3)

        hi, lo = data.max(), data.min()
        scaled = np.uint8((data - lo)/(hi - lo + 1e-3)*256)
        imsave(
            self.location, scaled, plugin=self.options.get("plugin")
        )
