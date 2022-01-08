# from ontopy import get_ontology # will soon release on pypi the bigg
# structure change in emmo-python

from emmo import get_ontology
import dlite

dlite.storage_path.append(f'../molecules/*.json')
dlite.storage_path.append(f'../reaction/*.json')

# input from ontologist
onto = get_ontology('https://raw.githubusercontent.com'
                    '/BIG-MAP/BattINFO/master/battinfo.ttl').load()


molecule_collection = dlite.Collection('json:../molecules/atomscaledata.json'
                                       '?mode=r#molecules',0) # not intuituve

reaction_collection = dlite.Collection('json:../reaction/reactiondata.json'
                                       '?mode=r#reactions',0) # not intuituve

'''
for inst in molecule_collection.get_instances():
    map: Molecule -> onto.Molecule
         Reaction -> onto.ChemicalReaction
         Reactant -> onto.Reactant
         Product -> onto.Product
         Energy -> onto.Energy # But we are talking about different energies
         here...

'''



