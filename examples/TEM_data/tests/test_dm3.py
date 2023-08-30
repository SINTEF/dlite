import json

import dlite

from paths import filenames, outdir


temfile = outdir / filenames[0]
dm3 = dlite.Instance.from_location("dm3", temfile)
print(dm3.meta)
metadata = json.loads(dm3.metadata)


# Test image analyser
from temdata.image_analyser import image_analyser
stat = image_analyser(dm3)
print(stat)
