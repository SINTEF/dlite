import sys
import json

import dlite

from paths import filenames, exampledir, outdir


Image = dlite.get_instance("http://onto-ns.com/meta/0.1/Image")

image = dlite.Instance.from_location("image", exampledir / "figs" / "040.png")
image.save("image", outdir / "tmp.png")

assert image.filename == str(exampledir / "figs" / "040.png")
assert image.data.shape == (256, 256, 4)
