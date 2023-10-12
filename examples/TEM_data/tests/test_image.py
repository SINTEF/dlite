from paths import filenames, exampledir, outdir

import dlite


image = dlite.Instance.from_location("image", exampledir / "figs" / "040.png")
image.save("image", outdir / "tmp.png")

assert image.filename == str(exampledir / "figs" / "040.png")
assert image.data.shape == (256, 256, 4)
