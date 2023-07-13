Storage plugin example
======================
This example shows how to write a user-defined Python storage plugin.

Let us assume that you have an instrument that logs a temperature at certain times.
The output file from a temperature profile measurement may look as follows:

```
measurements: 4
time  temperature
0     20
10    70
20    80
30    85
```

The first line provides the number of measurements performed in the experiment, followed by a table of time-temperature pairs for each temperature measurement.


Creating an entity
------------------
From the manual we know that the time is number of minutes since the start of the measurement and that the temperature is measured in degree Celsius.
This allows us to make a datamodel in the form of a DLite entity for this output:

```json
{
  "uri": "http://onto-ns.com/meta/0.1/TempProfile",
  "description": "Measured temperature profile.",
  "dimensions": {
    "n": "Number of temperature measurements."
  },
  "properties": {
    "time": {
      "type": "int",
      "shape": ["n"],
      "unit": "min",
      "description": "Number of minutes since the start of the experiment."
    },
    "temperature": {
      "type": "int",
      "shape": ["n"],
      "unit": "degC",
      "description": "Measured temperature at the given times."
    }
  }
}
```

This file will we place in a sub-directory called `entities/`.


Creating a storage plugin
-------------------------
We will now write a Python storage plugin that can instantiate a `TempProfile` entity from the output data of the instrument.
We will place this storage plugin in the `plugins/` sub-directory with the following content:

```python
"""Specific DLite storage plugin for time-temperature profile."""
from pathlib import Path
import dlite


class tempprofile(dlite.DLiteStorageBase):
    """DLite storage plugin for a temperature profile."""

    # At the time this plugin is importet, instantiate a TempProfile entity.
    # We keep a reference to this entity as a class attribute.
    TempProfile = dlite.Instance.from_location(
        "json", Path(__file__).resolve().parent.parent /
        "entities" / "TempProfile.json",
    )

    def open(self, location, options=None):
        """Opens temperature profile.

        Arguments:
            location: Path to temperature profile data file.
            options: Additional options for this driver.  Unused.
        """
        self.location = location

    def load(self, id=None):
        """Reads storage into an new instance and returns the instance.

        Arguments:
            id: Optional URI to assign to the new instance.

        Returns:
            A new TempProfile instance with the loaded temperature profile.
        """
        with open(self.location, "rt") as f:
            line = f.readline()
            n = int(line.split(":")[1].strip())

            # Create a new TempProfile instance with dimension `n` and the
            # provided ID.
            inst = self.TempProfile([n], id=id)

            f.readline()  # skip table header
            for i in range(n):
                time, temp = f.readline().split()
                inst.time[i] = int(time)
                inst.temperature[i] = int(temp)
        return inst

    def save(self, inst):
        """Stores `inst` to storage.

        Arguments:
            inst: A TempProfile instance to store.
        """
        n = inst.dimensions["n"]
        with open(self.location, "wt") as f:
            f.write(f"measurements: {n}\n")
            f.write(f"time  temperature\n")
            for time, temp in zip(inst.time, inst.temperature):
                f.write(f"{time:4} {temp:12}\n")
```

Some important points to note:
* Python storage plugins are imported by the Python interpreter that is embedded in DLite, not the interpreter that a user may import dlite from.
* Python storage plugins are created by subclassing `dlite.DLiteStorageBase`.
* The name of the subclass will become the *driver*  name for this plugin.

A `dlite.DLiteStorageBase` subclass may define the following methods:

* **open(self, location, options=None)**: required

  This method is called when a user create a storage using this plugin.
  The `location` and `options` arguments have the same meaning as the corresponding arguments to `dlite.Storage()`.

* **close(self)**: optional

  This method is called when a storage is closed.

* **flush(self)**: optional

  Flush cached data to storage. Called by the flush() of the storage.

* **load(self, id=None)**: optional

  Loads an instance identified by `id` from storage and returns it.
  Called by the load() method of the storage or by dlite.Instance.from_storage().

* **from_bytes(self, buffer, id=None)**: optional

  Load instance with given `id` from `buffer`.
  Plugin developers may allow omitting `id` if `buffer` only holds one instance.

* **save(self, inst)**: optional

  Saves instance `inst` to storage.
  Called by the save() method of the storage or instance.

* **to_bytes(self, inst)**: optional

  Returns instance `inst` as a bytes (or bytearray) object.

* **delete(self, uuid)**: optional

  Delete instance with given `uuid` from storage.

* **query(self, pattern=None)**: optional

  Query the storage for all instances who's metadata IRI matches the glob pattern `pattern`.
  If `metaid` is None, it queries all instances in the storage.

  Returns an iterator over the UUIDs of the matched instances.

If an optional method is not defined, the storage plugin does not support the corresponding feature.


Testing the new storage plugin
------------------------------
We are now ready to test the storage plugin.
A simple test is included in the [main.py] script.

This script first appends the `plugins/` sub-directory to the `dlite.python_storage_plugin_path` such that dlite is able to find the new plugin:

```python
>>> from pathlib import Path
>>> import dlite

>>> thisdir = Path(__file__).resolve().parent
>>> dlite.python_storage_plugin_path.append(thisdir  / "plugins")

```

`dlite.python_storage_plugin_path` is a special path object, with a list-like Python interface.
It is instantiated from the `DLITE_PYTHON_STORAGE_PLUGIN_DIRS` environment variable.

We can now load a TempProfile instance from `dataset.txt` with [^footnote]:

```python
>>> inst = dlite.Instance.from_location("tempprofile", "dataset.txt", "mode=r")
>>> print(inst)
{
  "uuid": "e3f36e98-3285-5fd0-b129-4635ac15ccdb",
  "uri": "ex:dataset",
  "meta": "http://onto-ns.com/meta/0.1/TempProfile",
  "dimensions": {
    "n": 4
  },
  "properties": {
    "time": [
      0,
      10,
      20,
      30
    ],
    "temperature": [
      20,
      70,
      80,
      85
    ]
  }
}

```

Since our plugin also defined a `save()` method, we can also save our instance to a new file called `newdata.txt`:

```python
>>> inst.save("tempprofile", "newdata.txt", "mode=w")

```

which will create a new file with the following content:

```
measurements: 4
time  temperature
   0           20
  10           70
  20           80
  30           85
```


---

[^footnote]: Note that in [main.py] is `dlite.Instance.from_location()` called as `dlite.Instance.from_location("tempprofile", thisdir / "dataset.txt", options="mode=r", id="ex:dataset")` with an absolute location and `id` as an additional argument.
This is not needed in this simple case, but a good practice.
The absolute location makes the example independent of the current working directory and the `id` argument allows the storage to contain multiple instances.


[main.py]: https://github.com/SINTEF/dlite/tree/master/examples/storage_plugin/main.py
