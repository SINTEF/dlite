import os

import dlite
from pymatgen.io.nwchem import NwOutput
from pymatgen.io.ase import AseAtomsAdaptor


data = NwOutput('nwchem_result1.nwout')

keys = data.data[0].keys()

