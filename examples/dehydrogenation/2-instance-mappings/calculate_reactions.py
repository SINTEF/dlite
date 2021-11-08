#!/usr/bin/env python
from pathlib import Path

import dlite
import reaction_energy


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
rootdir = thisdir.parent
atomdata = rootdir / '1-simple-workflow' / 'atomscaledata.json'
dlite.storage_path.append(f'{rootdir}/entities/*.json')
dlite.python_mapping_plugin_path.append(f'{rootdir}/python-mapping-plugins')


# Load collection with atom scale data
coll = dlite.Collection(f'json://{atomdata}?mode=r#molecules', 0)

# Define the reaction of interest and calculate the reaction energy
r = reaction_energy.reaction_energy(coll, reactants={'C2H6': 1},
                                        products={'C2H4': 1,'H2': 1})
print(f'Reaction energy: {r.energy} eV')

r.save(f'yaml://{thisdir}/reaction.yaml?mode=w')
