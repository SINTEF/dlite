import sys
import json

import requests

import dlite

from paths import filenames, outdir


temfile = outdir / filenames[0]
dm3 = dlite.Instance.from_location("dm3", temfile)

dm3.save("temsettings", outdir / "temsettings.json")
