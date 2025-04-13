"""Example Python script for loading and storing Measurement instances."""
from pathlib import Path
import dlite


# Set search path to our user-defined storage plugin
thisdir = Path(__file__).resolve().parent
#dlite.python_storage_plugin_path.append(thisdir  / "plugins")

dlite.storage_path.append(thisdir / "entities" / "TempProfile.json")
dlite.python_storage_plugin_path.append(thisdir  / "plugins")

DataModel = dlite.get_instance("http://onto-ns.com/meta/0.1/TempProfile")

# Create instance from dataset
inst = dlite.Instance.from_location("tempprofile", thisdir / "dataset.txt",
                                    options="mode=r", id="ex:dataset")
print(inst)

# Save instance to new dataset
inst.save("tempprofile", "newdata.txt", "mode=w")
