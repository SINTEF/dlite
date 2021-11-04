#!/usr/bin/env python3
from typing import Dict, AnyStr
from pathlib import Path
from ontopy import get_ontology

import dlite
from dlite.mappings import make_instance


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
rootdir = thisdir.parent.parent
workflow1dir = rootdir / '1-simple-workflow'
entitiesdir = rootdir / 'entities'
atomdata = workflow1dir / 'atomscaledata.json'
dlite.storage_path.append(f'{entitiesdir}/*.json')


# Define the calculation
def get_energy(reaction):
    """Calculates reaction energies with data from Substance entity
    data is harvested from collection and mapped to Substance according to
    mappings.

    Args:
        reaction: dict with names of reactants and products ase keys
                  and stochiometric coefficient as value
                  Negative stochiometric coefficients for reactants.
                  Positive stochiometric coefficients for products.
    Returns:
        reaction energy
    """
    energy = 0
    for label, n in reaction.items():
        inst = make_instance(Substance, coll[label], mappings,
                             mapsTo=mapsTo)
        energy+=n*inst.molecule_energy
    return energy



# Import ontologies with mappings
molecules_onto = get_ontology('mapping_mols.ttl').load()
reaction_onto = get_ontology('mapping_substance.ttl').load()

# Convert to mappings to a single list of triples
mappings = list(molecules_onto.get_unabbreviated_triples())
mappings.extend(list(reaction_onto.get_unabbreviated_triples()))

# Obtain the Metadata to be mapped to eachothers
Molecule = dlite.get_instance('http://onto-ns.com/meta/0.1/Molecule')
Substance = dlite.get_instance('http://onto-ns.com/meta/0.1/Substance')


mapsTo = 'http://onto-ns.com/ontology/mapsTo#mapsTo'

# Define where the molecule data is obtained from
# This is a dlite collection
coll = dlite.Collection(f'json://{atomdata}?mode=r#molecules', 0)


# input from chemical engineer, e.g. what are reactants and products
# reactants (left side of equation) have negative stochiometric coefficient
# products (right side of equation) have positive stochiometric coefficient
reaction1 = {'C2H6':-1, 'C2H4':1,'H2':1}

reaction_energy = get_energy(reaction1)
print('Reaction energy 1', reaction_energy)


reaction2 = {'C3H8':-1, 'H2': -2,'CH4':3}

reaction_energy2 = get_energy(reaction2)
print('Reaction energy 1', reaction_energy2)



# Map instance Molecule with label 'H2' to Substance
#inst = make_instance(Substance, coll['H2'], mappings)
#print(inst)

# Map instance Molecule with label 'H2' to itself
#inst2 = make_instance(Molecule, coll['H2'], mappings, strict=False)
#print(inst2)



