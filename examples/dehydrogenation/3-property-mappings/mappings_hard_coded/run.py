#!/usr/bin/env python3
from pathlib import Path

import dlite
from dlite.mappings import make_instance


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
rootdir = thisdir.parent.parent
workflow1dir = rootdir / '1-simple-workflow'
entitiesdir = rootdir / 'entities'
atomdata = workflow1dir / 'atomscaledata.json'
dlite.storage_path.append(f'{entitiesdir}/*.json')

# Define all mappings needed for mapping from Molecule to Substance
mappings = [
    ('http://onto-ns.com/meta/0.1/Molecule#name', ':mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Molecule#groundstate_energy', ':mapsTo',
     'chem:GroundStateEnergy'),
    ('http://onto-ns.com/meta/0.1/Substance#id', ':mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Substance#molecule_energy', ':mapsTo',
     'chem:GroundStateEnergy'),
]

Molecule = dlite.get_instance('http://onto-ns.com/meta/0.1/Molecule')
Substance = dlite.get_instance('http://onto-ns.com/meta/0.1/Substance')


def get_energy(reaction):
    """Calculates reaction energies with data from Substance entity
    data is harvested from collection and mapped to Substance according to
    mappings

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
        inst = make_instance(Substance, coll[label], mappings)
        energy += n*inst.molecule_energy
    return energy


# Define where the molecule data is obtained from
# This is a dlite collection
coll = dlite.Collection(f'json://{atomdata}?mode=r#molecules', 0)

# input from chemical engineer, e.g. what are reactants and products
# reactants (left side of equation) have negative stochiometric coefficient
# products (right side of equation) have positive stochiometric coefficient
reaction1 = {'C2H6': -1, 'C2H4': 1, 'H2': 1}

reaction_energy = get_energy(reaction1)
print('Reaction energy 1', reaction_energy)


reaction2 = {'C3H8':-1, 'H2': -2,'CH4': 3}

reaction_energy2 = get_energy(reaction2)
print('Reaction energy 1', reaction_energy2)
