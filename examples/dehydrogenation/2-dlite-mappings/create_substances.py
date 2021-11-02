#!/usr/bin/env python
from pathlib import Path

import dlite


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
rootdir = thisdir.parent
workflow1dir = rootdir / '1-simple-workflow'
entitiesdir = rootdir / 'entities'
atomdata = workflow1dir / 'atomscaledata.json'
dlite.storage_path.append(f'{entitiesdir}/*.json')
dlite.python_mapping_plugin_path.append(f'{rootdir}/python-mapping-plugins')

substance_id = 'http://onto-ns.com/meta/0.1/Substance'


def reaction_energy(coll, reactants, products):
    """Returns the calculated reaction energy.

    Args:
        coll: Collection with molecules.
        reactants: Dict with reactants.  It should map molecule names to
          their corresponding stoichiometric coefficient in the reaction
          formula.
        products: Dict with products.  It should map molecule names to
          their corresponding stoichiometric coefficient in the reaction
          formula.
    """
    energy = 0
    for label, n in reactants.items():
        print(f'--- label={label}')
        substance = coll.get(label, substance_id)
        energy -= n * substance.molecule_energy

    for label, n in products.items():
        print(f'+++ label={label}')
        substance = coll.get(label, substance_id)
        energy += n * substance.molecule_energy

    return energy


if __name__ == '__main__':
    Substance = dlite.get_instance(substance_id)


    # Load collection with atom scale data
    coll = dlite.Collection(f'json://{atomdata}?mode=r#molecules', 0)

    e = reaction_energy(coll, reactants={'C2H6': 1},
                        products={'C2H4': 1,'H2': 1})
    print(f'Reaction energy: {e} eV')


    #reactants={'C2H6':1}
    #products={'C2H4':1,'H2':1}
    #s1 = coll.get('C2H6', substance_id)
    #s2 = coll.get('C2H4', substance_id)
    #s3 = coll.get('H2', substance_id)
    #
    #energy = s2.molecule_energy + s3.molecule_energy - s1.molecule_energy
    #print(f'Reaction energy: {energy} eV')
