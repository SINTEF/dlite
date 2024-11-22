"""Test for memory leaks."""
import dlite


# Test proper reference counting of Collection.get_id()
coll = dlite.Collection()
coll2 = dlite.Collection(id="c2")
coll.add("coll2", coll2)

c = coll.get_id("c2")

assert coll2._refcount == 3

del c
assert coll2._refcount == 2

del coll
assert coll2._refcount == 1
