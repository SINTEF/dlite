from pathlib import Path

try:
    import pytest
except ImportError:
    HAVE_PYTEST = False
else:
    HAVE_PYTEST = True

import dlite


# Configure paths
thisdir = Path(__file__).parent.absolute()

#dlite.storage_path.append(thisdir / 'entities' / '*.json')
dlite.storage_path.append(thisdir / '*.json')
Person = dlite.get_instance('http://onto-ns.com/meta/0.1/Person')


person = Person(dims={"N": 4})
person.name = "Knud H. Thomsen"
person.age = 30
person.skills = ["writing", "humor", "history", "human knowledge"]

# Update person 6 times and create a snapshot before each change
for i in range(6):
    person.snapshot()
    person.age += 5

assert person.age == 60
assert person.get_snapshot(0).age == 60
assert person.get_snapshot(1).age == 55
assert person.get_snapshot(6).age == 30
if HAVE_PYTEST:
    with pytest.raises(dlite.DLiteError):
        person.get_snapshot(7)

assert person._get_parent_uuid() == person.get_snapshot(1).uuid
assert person._get_parent_hash() == person.get_snapshot(1).get_hash()

# Chech mutability
assert person.is_frozen() == False
assert person.get_snapshot(1).is_frozen() == True
assert person.get_snapshot(2).is_frozen() == True

# Create branch
inst = person.copy()
inst.set_parent(person.get_snapshot(3))
inst.age = 50
inst.snapshot()
inst.age = 55

assert inst.get_snapshot(0).age == 55
assert inst.get_snapshot(1).age == 50
assert inst.get_snapshot(2).age == 45
assert inst.get_snapshot(3).age == 40
assert inst.get_snapshot(4).age == 35
assert inst.get_snapshot(5).age == 30
if HAVE_PYTEST:
    with pytest.raises(dlite.DLiteError):
        inst.get_snapshot(6)

#print(inst)

# Push to storage
db = thisdir / "output" / "db.json"
db.unlink(missing_ok=True)  # Make sure that the db is empty
with dlite.Storage("json", db, "mode=w") as storage:
    #person.push_snapshot(storage, 1)
    storage.save(person)
