import dlite
import os

dlite.storage_path.append(f'../molecules/*.json')

# input from chemical engineer
reactants = {'c2h6':1}
products = {'c2h4':1,'h2':1}

molecule_collection = dlite.Collection('json:../molecules/atomscaledata.json?mode=r#molecules',0) # not intuituve

Reaction = dlite.Instance('json:Reaction.json')

reaction = Reaction(dims=[len(reactants), len(products)])

reaction.reactants = list(reactants.keys())
reaction.products = list(products.keys())

reaction.reactant_stoichiometric_coefficient = list(reactants.values())
reaction.product_stoichiometric_coefficient = list(products.values())

energy = 0
for label, n in reactants.items():
    inst = molecule_collection.get(label)
    energy-=n*inst.energy

for label, n in products.items():
    inst = molecule_collection.get(label)
    energy+=n*inst.energy

reaction.energy = energy


coll = dlite.Collection('reactions')
coll.add('ethane_dehydrogenation',reaction)
coll.save('json', 'reactiondata.json', 'mode=w')


