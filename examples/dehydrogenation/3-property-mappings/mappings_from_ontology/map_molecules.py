from pathlib import Path

from ontopy import get_ontology, World
from ontopy.utils import write_catalog


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
rootdir = thisdir.parent.parent


# Load ontologies into a common world
world = World()

mapsTo_onto = world.get_ontology(f'{rootdir}/ontology/mapsTo.ttl').load(
    emmo_based=False)

chemistry_onto = world.get_ontology(f'{rootdir}/ontology/chemistry.ttl').load()

dlite_onto = world.get_ontology(
    'https://raw.githubusercontent.com/emmo-repo/datamodel-ontology/master'
    '/metamodel.ttl').load(emmo_based=False)

mapping = world.get_ontology('http://onto-ns.com/ontology/mapping#')
mapping.set_version('0.1')
mapping.imported_ontologies.extend([mapsTo_onto, chemistry_onto, dlite_onto])


molecule = dlite_onto.Metadata()
molecule.iri = 'http://onto-ns.com/meta/0.1/Molecule'
molecule_energy = dlite_onto.Metadata()

molecule_energy.iri = ('http://onto-ns.com/meta/0.1/'
                       'Molecule#groundstate_energy')

molecule_name = dlite_onto.Metadata()
molecule_name.iri = 'http://onto-ns.com/meta/0.1/Molecule#name'

with mapping:
    molecule.mapsTo.append(chemistry_onto.MoleculeModel)
    molecule_energy.mapsTo.append(chemistry_onto.GroundStateEnergy)
    molecule_name.mapsTo.append(chemistry_onto.Identifier)


# XXX
onto = chemistry_onto
onto.Identifier.mapsTo.append(onto.Atom)
onto.BondedAtom.mapsTo.append(onto.Field)


mapping.save(f'{thisdir}/mapping_mols.ttl')

# Since the iris are not directly findable on the www, a catalog file
# with pointers to the imported ontologies must be made in order
# to ensure correct loading  of mapping_mols.ttl in EMMOntopy or Protege
catalog = {'http://emmo.info/datamodel/metamodel':
           'https://raw.githubusercontent.com/emmo-repo/datamodel-ontology/master/metamodel.ttl',
           'http://emmo.info/datamodel/0.0.2':
           'https://raw.githubusercontent.com/emmo-repo/datamodel-ontology/master/datamodel.ttl',
           'http://emmo.info/datamodel/0.0.2/entity':
           'https://raw.githubusercontent.com/emmo-repo/datamodel-ontology/master/entity.ttl',
           'http://onto-ns.com/ontology/chemistry': '../../ontology/chemistry.ttl',
           'http://onto-ns.com/ontology/mapsTo': '../../ontology/mapsTo.ttl'}
write_catalog(catalog, f'{thisdir}/catalog-v001.xml')




#import owlready2
#
#def related(onto, source, relation, route=[]):
#    """Returns a generator over all entities that `source` relates to via
#    `relation`.
#    """
#    for e1 in getattr(source, relation.name):
#        r1 = route + [(source.iri, relation.iri, e1.iri)]
#        for e2 in e1.descendants():
#            r2 = r1 + [(e2.iri, 'rdfs:subClassOf', e1.iri)] if e1 != e1 else r1
#            for rel in relation.descendants():
#                r3 = r2 + [(rel.iri, 'rdfs:subProperty', relation.iri)
#                           ] if rel != relation else r2
#                yield e2, r3
#                if issubclass(rel, owlready2.TransitiveProperty):
#                    yield from related(onto, e2, relation, r3)
#
#m = mapping.world['http://onto-ns.com/meta/0.1/Molecule#name']
#for e, r in related(mapping, m, mapping.mapsTo):
#    print()
#    print(e)
#    print(r)
