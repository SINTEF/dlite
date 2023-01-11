import os

from pymatgen.io.ase import AseAtomsAdaptor
from pymatgen.io.nwchem import NwOutput

import dlite

data = NwOutput("nwchem_result1.nwout")

keys = data.data[0].keys()
