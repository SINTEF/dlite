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
            - `plugin`: Name of scikit image io plugin to use for loading
              the image.  By default, the different plugins are tried
              (starting with imageio) until a suitable candidate is found.
              If not given and fname is a tiff file, the 'tifffile' plugin
              will be used.
            - `crop`: Crop out a part of the image.  Applied before other
              operations.  Specified as comma-separated set of ranges
              to crop out for each dimension. Example: for a 2D image will
              ":,50:150" keep the hight, but reduce the width to 100 pixes
              starting from x=50.
            - `as_gray`: Whether to convert color images to gray-scale
              when loading.  Default is false.
            - `equalize`: Whether to equalize histogram before saving.
              Default is false.
            - `resize`: Required image size when saving.  Should be given
              as `HEIGHTxWIDTH` (ex: "256x128").
            - `order`: Order of spline interpolation for resize. Default
              to zero for binary images and 1 otherwise.  Should be in the
              range 0-5.
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

        crop = self.options.get("crop")
        if crop:
            # We call __getitem__() explicitly, since the preferred syntax
            # `data[*toindex(crop)]` is not supported by Python 3.7
            data = data.__getitem__(*toindex(crop))

        # Infer dimensions
        shape = [1]*3
        shape[3 - len(data.shape):] = data.shape
        dimnames = "height", "width", "channels"
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

        crop = self.options.get("crop")
        if crop:
            # We call __getitem__() explicitly, since the preferred syntax
            # `data[*toindex(crop)]` is not supported by Python 3.7
            data = data.__getitem__(toindex(crop))

        if self.options.get("resize"):
            size = [int(s) for s in self.options.resize.split("x")]
            shape = list(data.shape)
            shape[:len(size)] = size
            kw = {}
            if self.options.get("order"):
                kw["order"] = int(self.options.order)
            data = resize(data, output_shape=shape, **kw)

        hi, lo = data.max(), data.min()
        scaled = np.uint8((data - lo)/(hi - lo + 1e-3)*256)
        imsave(
            self.location, scaled, plugin=self.options.get("plugin")
        )


def toindex(crop_string):
    """Convert a crop string to a valid numpy index.

    Example usage: `arr[*toindex(":,50:150")]`
    """
    ranges = []
    for dim in crop_string.split(","):
        if dim == ":":
            ranges.append(slice(None))
        else:
            ranges.append(slice(*[int(d) for d in dim.split(":")]))
    return tuple(ranges)
