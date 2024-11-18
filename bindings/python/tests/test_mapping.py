#!/usr/bin/env python
from pathlib import Path

import dlite


# Configure search paths
thisdir = Path(__file__).parent.absolute()
entitydir = thisdir / "entities"
dlite.storage_path.append(f"{entitydir}/*.json")
dlite.python_mapping_plugin_path.append(f"{thisdir}/python-mapping-plugins")

# Create an instance of Person
Person = dlite.Instance.from_url(f"json:{entitydir}/Person.json?mode=r")
person = Person(dimensions=[2])
person.name = "Neil Armstrong"
person.age = 39
person.skills = ["keping the head cold", "famous quotes"]

print(f"*** person: {person.uuid}: refcount: {person._refcount}")

# Map person to an instance of SimplePerson
simple = dlite.mapping("http://onto-ns.com/meta/0.1/SimplePerson", [person])
assert simple != person
assert simple.name == person.name
assert simple.age == person.age

print(f"*** person: {person.uuid}: refcount: {person._refcount}")
print(f"*** simple: {simple.uuid}: refcount: {simple._refcount}")

import sys
sys.exit()

# Add the instance of SimplePerson to a collection
coll = dlite.Collection()
coll.add("simple", simple)

# Get the added person instance from the collection mapped to a new
# instance of SimplePerson (the second argument can be omitted...)
s = coll.get("simple", "http://onto-ns.com/meta/0.1/SimplePerson")
assert s == simple
s2 = coll.get("simple")
assert s2 == s

# Get the added person instance from the collection mapped to a new
# instance of Person (with no skills)
p = coll.get("simple", "http://onto-ns.com/meta/0.1/Person")
assert p != person
assert p.meta == person.meta
assert p.name == person.name
assert p.age == person.age
