#!/usr/bin/env python3
import os
from pathlib import Path

import dlite
import ase.io
from ase.calculators.emt import EMT


# Setup
thisdir = Path(__file__).parent.absolute()
moldir = thisdir.parent / 'molecules'  # directory with .xyz files
entitydir = thisdir.parent / 'entities'


def readMolecule(filename):
    """Reads molecule structure from `filename` and return it as an
    instance of Molecule.

    ASE is used to calculate the molecule ground state energy.
    """
    atoms = ase.io.read(filename)  # ASE Atoms object
    atoms.calc = EMT()
    molname = Path(filename).stem
    inst = Molecule(dims=[len(atoms), 3], id=molname)  # DLite instance
    inst.name = molname
    inst.positions = atoms.positions
    inst.symbols = atoms.get_chemical_symbols()
    inst.masses = atoms.get_masses()
    inst.energy = atoms.get_potential_energy()
    return inst


Molecule = dlite.Instance(f'json://{entitydir}/Molecule.json')  # DLite Metadata

# Create a new collection and populate it with all molecule structures
coll = dlite.Collection('molecules')
for filename in moldir.glob('*.xyz'):
    molname = filename.stem
    mol = readMolecule(filename)
    coll.add(label=molname, inst=mol)

coll.save('json', f'{thisdir}/atomscaledata.json', 'mode=w')

# Change this example so that the calculation is done on the entities
# from the collection of Molecules (with only info about chemical structure)
# Read and populate the Molecule
