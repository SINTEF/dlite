import sys
import json

import dlite

from paths import filenames, indir, outdir


temfile = outdir / filenames[0]
outfile = outdir / "temsettings.json"
infile = indir / "040.json"


dm3 = dlite.Instance.from_location("dm3", temfile)
dm3.save("temsettings", outfile)


assert outfile.read_text() == infile.read_text()
