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

    substance_model = dlite_onto.Metadata('http://onto-ns.com/meta/0.1/Substance')
    substance_model.mapsTo.append(chemistry_onto.MoleculeModel)


    substance_energy = dlite_onto.Metadata('http://onto-ns.com/meta/0.1/Substance#energy')
    substance_energy.mapsTo.append(chemistry_onto.GroundStateEnergy)

mapping.save('mapping_reaction.ttl')
