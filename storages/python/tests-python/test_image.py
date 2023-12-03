from pathlib import Path

import dlite


thisdir = Path(__file__).absolute().parent
indir = thisdir / 'input'
outdir = thisdir / 'output'
#entitydir = thisdir.parent / "python-storage-plugins"
#dlite.storage_path.append(entitydir)

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
