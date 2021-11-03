from ontopy import get_ontology, World
from ontopy.utils import write_catalog

# Note install emmopython from github, not pypi.
world = World()

mapsTo_onto = world.get_ontology('../ontology/mapsTo.ttl').load(EMMObased=False)

chemistry_onto = world.get_ontology('../ontology/chemistry.ttl').load()

dlite_onto = world.get_ontology('https://raw.githubusercontent.com/'
                          'emmo-repo/datamodel-ontology/master'
                          '/dlitemodel.ttl').load(EMMObased=False)
mapping = world.get_ontology('http://onto-ns.com/ontology/mapping#')
mapping.set_version('0.1')
#mapping.base_iri = 'http://onto-ns.com/ontology/mapping#'
mapping.imported_ontologies.extend([mapsTo_onto, chemistry_onto, dlite_onto])

#with mapping:

molecule = dlite_onto.Metadata() #'http://onto-ns.com/meta/0.1/Molecule')
molecule.iri = 'http://onto-ns.com/meta/0.1/Molecule'
molecule_energy = dlite_onto.Metadata()

molecule_energy.iri = ('http://onto-ns.com/meta/0.1/'
                       'Molecule#groundstate_energy')

molecule_name = dlite_onto.Metadata() #'http://onto-ns.com/meta/0.1/Molecule')
molecule_name.iri = 'http://onto-ns.com/meta/0.1/Molecule#name'

with mapping:
    molecule.mapsTo.append(chemistry_onto.MoleculeModel)

    molecule_energy.mapsTo.append(chemistry_onto.GroundStateEnergy)

    molecule_name.mapsTo.append(chemistry_onto.Identifier)


mapping.save('mapping_mols.ttl')

catalog = {'http://emmo.info/datamodel/dlitemodel':
           'https://raw.githubusercontent.com/emmo-repo/datamodel-ontology/master/dlitemodel.ttl',
           'http://emmo.info/datamodel/0.0.1':
           'https://raw.githubusercontent.com/emmo-repo/datamodel-ontology/master/datamodel.ttl',
           'http://emmo.info/datamodel/0.0.1/entity':
           'https://raw.githubusercontent.com/emmo-repo/datamodel-ontology/master/entity.ttl',
           'http://onto-ns.com/ontology/chemistry': '../ontology/chemistry.ttl',
           'http://onto-ns.com/ontology/mapsTo': '../ontology/mapsTo.ttl'}
write_catalog(catalog, 'catalog-v001.xml')
