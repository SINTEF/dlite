import sys
import json

import requests

import dlite

from paths import filenames, exampledir, outdir


Image = dlite.get_instance("http://onto-ns.com/meta/0.1/Image")

image = dlite.Instance.from_location("image", exampledir / "figs" / "040.png")
image.save("image", outdir / "tmp.png")
