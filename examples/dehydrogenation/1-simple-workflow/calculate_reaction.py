#!/usr/bin/env python3
from pathlib import Path

import dlite


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
entitydir = thisdir.parent / 'entities'
reactiondir = thisdir
mappingdir = thisdir.parent / 'python-mapping-plugins'
atomdata = thisdir / 'atomscaledata.json'
dlite.storage_path.append(f'{entitydir}/*.json')
dlite.python_mapping_plugin_path.append(f'{mappingdir}')

# input from chemical engineer
reactants = {'C2H6': 1}
products = {'C2H4': 1, 'H2': 1}

coll = dlite.Collection.create_from_url(f'json://{atomdata}?mode=r#molecules')

Reaction = dlite.Instance.create_from_url(f'json://{entitydir}/Reaction.json')

reaction = Reaction(dims=[len(reactants), len(products)])

reaction.reactants = list(reactants.keys())
reaction.products = list(products.keys())

reaction.reactant_stoichiometric_coefficient = list(reactants.values())
reaction.product_stoichiometric_coefficient = list(products.values())

energy = 0
for label, n in reactants.items():
    inst = coll.get(label)
    energy -= n*inst.groundstate_energy

for label, n in products.items():
    inst = coll.get(label)
    energy += n*inst.groundstate_energy

print(f'Calculated reaction energy: {energy:.3} eV')
reaction.energy = energy

reaction.save('json', f'{thisdir}/ethane-dehydrogenation.json', 'mode=w')
