#!/usr/bin/env python
from pathlib import Path

import dlite
from dlite.datamodel import DataModel


datamodel = DataModel("http://onto-ns/meta/0.1/Atoms")
datamodel.description = "A test entity for atoms..."
datamodel.add_dimension("natoms", "Number of atoms.")
datamodel.add_dimension("ncoords", "Number of coordinates (always 3).")
datamodel.add_dimension("nvecs", "Number of lattice vectors (always 3).")
datamodel.add_property(
    "symbol", "string", ["natoms"], description="Chemical symbol of each atom."
)
datamodel.add_property(
    "positions",
    "float",
    ["natoms", "ncoords"],
    description="Position of each atom.",
)
datamodel.add_property(
    "unitcell", "float", ["nvecs", "ncoords"], description="Unit cell."
)
Atoms = datamodel.get()


#atoms = Atoms(dimensions=[2, 3, 3])
atoms = Atoms(dimensions=dict(nvecs=3, ncoords=3, natoms=2))
