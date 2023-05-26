#!/usr/bin/env python
# -*- coding: utf-8 -*-
from pathlib import Path

import dlite


thisdir = Path(__file__).resolve().parent
rootdir = thisdir.parent.parent.parent


class Person:
    def __init__(self, name, age, skills):
        self.name = name
        self.age = age
        self.skills = skills

    def __repr__(self):
        return 'Person(%r, %r, %r)' % (self.name, self.age, list(self.skills))


url = f'json://{thisdir}/Person.json'

print('-- create: ExPerson')
ExPerson = dlite.classfactory(Person, url=url)

print('-- create: person1')
person1 = Person('Jack Daniel', 42, ['distilling', 'tasting'])

print('-- create: person2')
person2 = ExPerson('Jack Daniel', 42, ['distilling', 'tasting'])
person2.dlite_inst.save('json', 'persons.json', 'mode=w')

# Print json-representation of person2 using dlite
print(person2.dlite_inst.asjson(indent=2))

inst = dlite.Instance.from_url('json://persons.json')
person3 = dlite.instancefactory(Person, inst)

person4 = dlite.objectfactory(person1, meta=person2.dlite_meta)


# Test for issue #523
import numpy as np

class Atoms:
    def __init__(self, symbols, positions, masses, energy):
        self.name = "Atoms"
        self.positions = positions
        self.symbols = symbols
        self.masses = masses
        self.groundstate_energy = energy

atoms = Atoms(
    symbols=["Al"]*4,
    positions=[
        [0.0, 0.0, 0.0],
        [0.5, 0.5, 0.0],
        [0.5, 0.0, 0.5],
        [0.0, 0.5, 0.5],
    ],
    masses=[26.98]*4,
    energy=2.54,
)

entitydir = rootdir / "examples/dehydrogenation/entities/*.json"
dlite.storage_path.append(entitydir)

Molecule = dlite.get_instance("http://onto-ns.com/meta/0.1/Molecule")
molecule = dlite.objectfactory(atoms, meta=Molecule)

assert molecule.dlite_inst.positions.tolist() == atoms.positions
