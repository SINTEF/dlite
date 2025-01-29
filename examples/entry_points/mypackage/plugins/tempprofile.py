"""Specific DLite storage plugin for time-temperature profile."""
from pathlib import Path
import dlite


class tempprofile(dlite.DLiteStorageBase):
    """DLite storage plugin for a temperature profile."""

    # At the time this plugin is importet, instantiate a TempProfile entity.
    # We keep a reference to this entity as a class attribute.
    with open(Path(__file__).resolve().parent.parent / "datamodels" /
              "TempProfile.json", "r") as f:
        TempProfile = dlite.Instance.from_json(f.read())

    def open(self, location, options=None):
        """Opens temperature profile.

        Arguments:
            location: Path to temperature profile data file.
            options: Additional options for this driver.  Unused.
        """
        self.location = location

    def close(self):
        """Closes the storage.  Unused in this case."""

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
