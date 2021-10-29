import os

import dlite
import ase.io
from ase.calculators.emt import EMT

def readMolecule(filename, Molecule):
    atoms = ase.io.read('../molecules/'+filename)  # ASE Atoms object
    atoms.calc = EMT()
    basename = os.path.splitext(filename)[0]
    inst = Molecule(dims=[len(atoms), 3], id=basename)  # DLite instance
    inst.name = basename
    inst.positions = atoms.positions
    inst.symbols = atoms.get_chemical_symbols()
    inst.masses = atoms.get_masses()
    inst.energy = atoms.get_potential_energy()
    return inst

Molecule = dlite.Instance('json:Molecule.json')  # DLite Metadata

coll = dlite.Collection('molecules')
for molname in ['c2h6', 'c2h4', 'h2', 'c3h6', 'c3h8', 'ch4']:
    mol = readMolecule(molname+'.xyz', Molecule)
    coll.add(label=molname, inst=mol)

coll.save('json', 'atomscaledata.json', 'mode=w')

# Change this example so that the calculation is done on the entities
# from the collection of Molecules (with only info about chemical structure)
# Read and populate the Molecule
