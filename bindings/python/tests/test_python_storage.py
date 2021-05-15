import os

import dlite


thisdir = os.path.dirname(__file__)

url = 'json://' + os.path.join(thisdir, 'Person.json')
Person = dlite.Instance(url)

person = Person(dims=[2])
person.name = 'Ada'
person.age = 12.5
person.skills = ['skiing', 'jumping']


print('=== saving...')
with dlite.Storage('yaml', 'test.yaml', 'mode=w') as s:
    s.save(person)


print('=== loading...', person.uuid)
with dlite.Storage('yaml', 'test.yaml', 'mode=r') as s:
    inst = s.load(id=person.uuid)
print(inst)

person2 = Person(dims=[3])
person2.name = 'Berry'
person2.age = 24.3
person2.skills = ['eating', 'sleeping', 'reading']
with dlite.Storage('yaml://test.yaml') as s:
    s.save(person2)


s = dlite.Storage('yaml://test.yaml')
uuids = s.get_uuids()
del s
del uuids
