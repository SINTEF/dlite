from pathlib import Path

import dlite


# Configure paths
thisdir = Path(__file__).parent.absolute()

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

assert person.age == 30 + 6 * 5  # 60
for i in range(7):
    assert person.get_snapshot(i).age == 30 + (6 - i) * 5
try:
    person.get_snapshot(7)
except dlite.DLiteError:
    pass
else:
    raise Exception(
        "Should've failed test (getting non-existant snapshot), but didn't."
    )
assert person._get_parent_uuid() == person.get_snapshot(1).uuid
assert person._get_parent_hash() == person.get_snapshot(1).get_hash()

# Check mutability
assert person.is_frozen() == False
assert person.get_snapshot(1).is_frozen() == True
assert person.get_snapshot(2).is_frozen() == True

frozen = person.get_snapshot(1)
assert frozen.is_frozen() == True
try:
    frozen.age = 77
except dlite.DLiteError:
    pass
else:
    raise Exception("frozen instance should not accept attribute assignment")

try:
    frozen.skills[0] = "skiing"
except ValueError:
    pass
else:
    raise Exception("frozen property array should not accept assignment")

# Create branch
inst = person.copy()
inst.set_parent(person.get_snapshot(3))
inst.age = 50
inst.snapshot()
inst.age = 55
for i in range(6):
    assert inst.get_snapshot(i).age == 55 - i*5

for i in range(4):
    assert inst.get_snapshot(i + 2).age == person.get_snapshot(3 + i).age

try:
    inst.get_snapshot(6)
except dlite.DLiteError:
    pass
else:
    raise Exception(
        "Should've failed test (getting non-existant snapshot), but didn't"
    )

# Push to storage
db = thisdir / "output" / "db.json"
if db.exists():
    db.unlink()  # Make sure that the db is empty

with dlite.Storage("json", db, "mode=w") as storage:
    #person.push_snapshot(storage, 1)
    storage.save(person)
