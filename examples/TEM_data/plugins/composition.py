"""DLite storage plugin for reading composition from CSV or Excel sheets."""
import numpy as np

import dlite
from dlite.options import Options


class composition(dlite.DLiteStorageBase):
    """DLite storage plugin for reading alloy compositions.

    Arguments:
        location: Path to composition file, typically a CSV or Excel file.
        options: Options passed on to the 'csv' driver.

    """
    meta = "http://onto-ns.com/meta/0.1/Composition"

    def open(self, location, options=None):
        """Loads file from `location`."""
        self.location = location
        self.options = options

    def load(self, id=None):
        """Returns Composition instance."""
        # Load data and metadata
        data = dlite.Instance.from_location("csv", self.location, self.options)
        props = list(data.properties.items())
        rowmask = props[0][1] == props[0][1][0]
        props = [(t[0], t[1][rowmask]) for t in props]
        nphases = rowmask.sum() - 1
        nelements = len(data.properties) - 2

        if not "nominal" in props[1][1]:
            raise ValueError(f"expected one phase named 'nominal' in file: '{self.location}'")

        Comp = dlite.get_instance(self.meta)
        comp = Comp(dimensions={"nphases": nphases, "nelements": nelements})
        comp.alloy = props[0][1][0]
        comp.elements = [t[0] for t in props[2:]]
        comp.phases = props[1][1][1:]
        comp.nominal_composition = [t[1][0] for t in props[2:]]
        for i in range(nphases):
            comp.phase_compositions[i] = [t[1][i+1] for t in props[2:]]

        return comp
