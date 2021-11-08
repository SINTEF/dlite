#!/usr/bin/env python
from pathlib import Path

import dlite


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
rootdir = thisdir.parent
atomdata = rootdir / '1-simple-workflow' / 'atomscaledata.json'
dlite.storage_path.append(f'{rootdir}/entities/*.json')

# URI to DLite metadata
substance_id = 'http://onto-ns.com/meta/0.1/Substance'
reaction_id = 'http://onto-ns.com/meta/0.1/Reaction'


def reaction_energy(coll, reactants, products):
    """Returns the calculated reaction energy.

    Args:
        coll: Collection with Substance instances.
        reactants: Dict with reactants.  It should map molecule names to
          their corresponding stoichiometric coefficient in the reaction
          formula.
        products: Dict with products.  It should map molecule names to
          their corresponding stoichiometric coefficient in the reaction
          formula.
    """
    energy = 0
    for label, n in reactants.items():
        substance = coll.get(label, substance_id)
        energy -= n * substance.molecule_energy

    for label, n in products.items():
        substance = coll.get(label, substance_id)
        energy += n * substance.molecule_energy

    # Instantiate a new Reaction instance
    Reaction = dlite.get_instance(reaction_id)
    reaction = Reaction(dims=[len(reactants), len(products)])
    reaction.reactants = list(reactants.keys())
    reaction.products = list(products.keys())
    reaction.reactant_stoichiometric_coefficient = list(reactants.values())
    reaction.product_stoichiometric_coefficient = list(products.values())
    reaction.energy = energy

    return reaction
