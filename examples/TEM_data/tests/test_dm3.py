import json

from paths import filenames, outdir

from numpy import allclose

import dlite


temfile = outdir / filenames[0]
dm3 = dlite.Instance.from_location("dm3", temfile)
print(dm3.meta)
metadata = json.loads(dm3.metadata)

assert dm3.ndim == 4
assert dm3.zSize2 == 1
assert dm3.zSize == 1
assert dm3.ySize == 2048
assert dm3.xSize == 2048
assert dm3.filename == str(temfile)
assert dm3.data.shape == (1, 1, 2048, 2048)
assert dm3.pixelUnit.tolist() == ['', '', 'nm', 'nm']
assert allclose(dm3.pixelSize, [0.0, 0.0, 0.052593052, 0.052593052])
assert isinstance(metadata, dict)


# Test image analyser
from temdata.image_analyser import image_analyser
stat = image_analyser(dm3)
print(stat)

assert stat.nconditions == 1
assert stat.alloy == ["Al-Mg-Si"]
assert stat.condition == ["A"]
assert stat.precipitate == ['beta"']
assert stat.number_density == [30000]
assert stat.avg_length == [50]
assert stat.avg_crossection == [11]
assert stat.volume_fraction == [2]
assert stat.avg_atomic_volume == [0.019]
