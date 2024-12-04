import sys
from pathlib import Path

import numpy as np

import dlite
from dlite.testutils import importskip

importskip("scipy")
importskip("skimage")


thisdir = Path(__file__).absolute().parent
indir = thisdir / 'input'
outdir = thisdir / 'output'
#entitydir = thisdir.parent / "python-storage-plugins"
#dlite.storage_path.append(entitydir)

# Test save
image = dlite.Instance.from_location("image", indir / "image.png")
image.save("image", outdir / "image.png")
image.save("image", outdir / "image-crop.png", "crop=60:120,60:120")
image.save("image", outdir / "image-cropy.png", "crop=60:120")
image.save("image", outdir / "image-eq.png", "equalize=true")
image.save("image", outdir / "image-resize.png", "resize=128x128")
image.save("image", outdir / "image-resize2.png", "resize=512x512")
image.save("image", outdir / "image-resize3.png", "resize=512x512;order=3")

assert image.filename == str(indir / "image.png")
assert image.data.shape == (256, 256, 4)

# Test load
im = dlite.Instance.from_location("image", outdir / "image.png")
assert im.data.shape == image.data.shape
assert np.all(im.data == image.data)
assert Path(im.filename).name == Path(image.filename).name

im = dlite.Instance.from_location("image", outdir / "image-crop.png")
assert im.data.shape == (60, 60, 4)

im = dlite.Instance.from_location("image", outdir / "image-cropy.png")
assert im.data.shape == (60, 256, 4)

im = dlite.Instance.from_location("image", outdir / "image-eq.png")
assert im.data.shape == (256, 256, 4)

im = dlite.Instance.from_location("image", outdir / "image-resize.png")
assert im.data.shape == (128, 128, 4)

im = dlite.Instance.from_location("image", outdir / "image-resize2.png")
assert im.data.shape == (512, 512, 4)

im = dlite.Instance.from_location("image", outdir / "image-resize3.png")
assert im.data.shape == (512, 512, 4)
