from ontopy import get_ontology, World

# Note install emmopython from github, not pypi. 
world = World()

mapsTo_onto = world.get_ontology('../ontology/mapsTo.ttl').load(EMMObased=False)

chemistry_onto = world.get_ontology('../ontology/chemistry.ttl').load()

dlite_onto = world.get_ontology('https://raw.githubusercontent.com/'
                          'emmo-repo/datamodel-ontology/master'
                          '/dlitemodel.ttl').load(EMMObased=False)

mapping = world.get_ontology('mapping.ttl')
mapping.imported_ontologies.extend([mapsTo_onto, chemistry_onto, dlite_onto])

with mapping:

    molecule = dlite_onto.Metadata('http://onto-ns.com/meta/0.1/Molecule')
    molecule.mapsTo = chemistry_onto.MoleculeModel


    molecule_energy = dlite_onto.Metadata('http://onto-ns.com/meta/0.1/'
                                          'Molecule#ground_state_energy')
    molecule_energy.mapsTo = chemistry_onto.GroundStateEnergy

mapping.save('mapping.ttl')




