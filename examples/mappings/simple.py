"""Mapping example using a collection - without mapping functions."""
from pathlib import Path

from tripper import EMMO, MAP, RDFS, Triplestore

import dlite


# Paths
thisdir = Path(__file__).absolute().parent
datadir = thisdir / "data"
entitydir = thisdir / "entities"

dlite.storage_path.append(f"{entitydir}/*.json")


# Create collection -- our knowledge base in this example
coll = dlite.Collection()

# Create triplestore "view" of our knowledge base
ts = Triplestore(backend="collection", collection=coll)

# Namespaces
AT = ts.bind("at", "http://onto-ns.com/meta/0.1/Structure#")
MOL = ts.bind("mol", "http://onto-ns.com/meta/0.1/Molecule#")
RES = ts.bind("res", "http://onto-ns.com/meta/0.1/CalcResult#")
EN = ts.bind("en", "http://onto-ns.com/meta/0.1/Energy#")
FS = ts.bind("fs", "http://onto-ns.com/meta/0.1/Forces#")
EX = ts.bind("ex", "http://onto-ns.com/meta/0.1/example-ontology#")

# Add mappings
ts.add_mapsTo(EMMO.Force, FS.forces)
ts.add_mapsTo(EMMO.PotentialEnergy, EN.energy)


ts.add_mapsTo(EMMO.PotentialEnergy, RES.potential_energy)
ts.add_mapsTo(EMMO.Force, RES.forces)


# Load data
Energy = dlite.get_instance(EN)
energy = Energy()
energy.energy = 2.1  # eV
coll.add('energy', energy)

Forces = dlite.get_instance(FS)
forces = Forces({'natoms': 2, 'ncoords': 3})
forces.forces = [
    (0.0, 0.0, -1.2),
    (0.0, 0.0, +1.2),
]
coll.add('forces', forces)



# Now, instantiate results from collection
result, = coll.get_instances(
    metaid=RES, property_mappings=True, function_repo=ts.function_repo
)
print(result)
