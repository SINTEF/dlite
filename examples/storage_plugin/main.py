"""Example Python script for loading and storing Measurement instances."""
from pathlib import Path
import dlite


# Set search path to our user-defined storage plugin
thisdir = Path(__file__).resolve().parent
dlite.python_storage_plugin_path.append(thisdir  / "plugins")

# Create instance from dataset
inst = dlite.Instance.from_location("tempprofile", thisdir / "dataset.txt",
                                    options="mode=r", id="ex:dataset")
print(inst)

# Save instance to new dataset
inst.save("tempprofile", "newdata.txt", "mode=w")
