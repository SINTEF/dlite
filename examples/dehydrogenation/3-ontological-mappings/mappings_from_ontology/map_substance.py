from ontopy import get_ontology, World

# Note install emmopython from github, not pypi.
world = World()

mapsTo_onto = world.get_ontology('../../ontology/mapsTo.ttl').load(EMMObased=False)

chemistry_onto = world.get_ontology('../../ontology/chemistry.ttl').load()

dlite_onto = world.get_ontology('https://raw.githubusercontent.com/'
                          'emmo-repo/datamodel-ontology/master'
                          '/dlitemodel.ttl').load(EMMObased=False)

mapping = world.get_ontology('http://onto-ns.com/ontology/mapping#')
mapping.set_version('0.1')
mapping.imported_ontologies.extend([mapsTo_onto, chemistry_onto, dlite_onto])

substance_model = dlite_onto.Metadata()
substance_model.iri = 'http://onto-ns.com/meta/0.1/Substance'

substance_energy = dlite_onto.Metadata()
substance_energy.iri = 'http://onto-ns.com/meta/0.1/Substance#molecule_energy'

substance_id = dlite_onto.Metadata()
substance_id.iri = 'http://onto-ns.com/meta/0.1/Substance#id'


with mapping:
    substance_model.mapsTo.append(chemistry_onto.MoleculeModel)
    substance_energy.mapsTo.append(chemistry_onto.GroundStateEnergy)
    substance_id.mapsTo.append(chemistry_onto.Identifier)

mapping.save('mapping_substance.ttl')

# A catalog file is not writte here because the catalog from molecule
# can be reused. This will most likely not be the case in a more realistic
# example where the different mappings will reside in different places
# (folders)
