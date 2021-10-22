from ontopy import get_ontology

# Note install emmopython from github, not pypi. 

mapping_onto = get_ontology('mapping.ttl').load(EMMObased=False)

chemistry_onto = get_ontology('chemistry.ttl').load()

dlite_onto = get_ontology('https://raw.githubusercontent.com/'
                          'emmo-repo/datamodel-ontology/master'
                          '/dlitemodel.ttl').load(EMMObased=False)



molecule_model = dlite_onto.Metadata('http://onto-ns.com/meta/0.1/Molecule')
molecule_model.mapsTo = chemistry_onto.MoleculeModel


