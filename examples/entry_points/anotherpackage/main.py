"""Example Python script for loading and storing Measurement instances."""
import sys

# The `importlib.resources.files()` function requires Python 3.10
if sys.version_info < (3, 10):
    sys.exit(44)

from importlib.resources import files

import dlite
from dlite.testutils import importskip

mypackage = importskip("mypackage")


datamodels = files('mypackage.datamodels')
datadir = files('mypackage.data')


# Create instance from dataset
inst = dlite.Instance.from_location("tempprofile", datadir / "dataset.txt",
                                    options="mode=r", id="ex:dataset")
print(inst)
#
# # Save instance to new dataset
# inst.save("tempprofile", "newdata.txt", "mode=w")
