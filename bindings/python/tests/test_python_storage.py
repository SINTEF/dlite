import os

import dlite


thisdir = os.path.dirname(__file__)

url = 'json://' + os.path.join(thisdir, 'Person.json')  #+ "?mode=r"
Person = dlite.Instance(url)

person = Person(dims=[2])
person.name = 'Ada'
person.age = 12.5
person.skills = ['skiing', 'jumping']


print('=== saving...')
with dlite.Storage('yaml', 'test.yaml', 'mode=w') as s:
    s.save(person)

print('=== loading...')
with dlite.Storage('yaml', 'test.yaml', 'mode=r') as s:
    inst = s.load(id=person.uuid)

print(inst)
