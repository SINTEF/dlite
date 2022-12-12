#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

import dlite

thisdir = os.path.abspath(os.path.dirname(__file__))


class Person:
    def __init__(self, name, age, skills):
        self.name = name
        self.age = age
        self.skills = skills

    def __repr__(self):
        return 'Person(%r, %r, %r)' % (self.name, self.age, list(self.skills))


url = 'json://' + thisdir + '/Person.json'

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
