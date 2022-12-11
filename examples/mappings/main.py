"""Mapping example using a collection."""
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


#ts.add_mapsTo(EX.ChemicalSymbol, AT.symbols)
ts.add_mapsTo(EMMO.PotentialEnergy, RES.potential_energy)
ts.add_mapsTo(EMMO.Force, RES.forces)
#
#ts.add_mapsTo(EX.Formula, MOL.formula)
#ts.add_mapsTo(EX.MaxForce, MOL.maxforce)
#ts.add_mapsTo(EMMO.PotentialEnergy, MOL.energy)
#
## Relate new concepts in example-ontology to EMMO
#ts.add_triples(
#    [
#        (EX.ChemicalSymbol, RDFS.subClassOf, EMMO.ChemicalElement),
#        (EX.ForceNorm, RDFS.subClassOf, EMMO.Force),
#        (EX.MaxForce, RDFS.subClassOf, EMMO.Force),
#    ]
#)
#
## Add mapping functions
#def formula(symbols):
#    """Convert a list of atomic symbols to a chemical formula."""
#    lst = symbols.tolist()
#    return "".join(f"{c}{lst.count(c)}" for c in set(lst))
#
#
#def norm(array, axis=-1):
#    """Returns the norm array along the given axis (default the last)."""
#    # Note that `array` is a Quantity object.  The returned value
#    # will also be a Quantity object with the same unit.  Hence, the
#    # unit is always handled explicitly.  This makes it possible for
#    # conversion function to change unit as well.
#    return np.sqrt(np.sum(array**2, axis=axis))
#
#
#def max(vector):
#    """Returns the largest element."""
#    return np.max(vector)
#
#ts.add_function(
#    formula,
#    expects=[EX.ChemicalSymbol],
#    returns=[EX.Formula],
#)
#ts.add_function(
#    norm,
#    expects=[EMMO.Force],
#    returns=[EX.ForceNorm],
#)
#ts.add_function(
#    max,
#    expects=[EX.ForceNorm],
#    returns=[EX.MaxForce],
#)


# Now, instantiate molecule
result, = coll.get_instances(
    metaid=RES, property_mappings=True, function_repo=ts.function_repo
)
print(result)



#molecule, = coll.get_instances(
#    metaid=MOL, property_mappings=True, function_repo=ts.function_repo
#)
#print(molecule)
