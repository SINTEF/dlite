"""Test for memory leaks."""
import dlite


coll = dlite.Collection()
coll2 = dlite.Collection(id="c2")
coll.add("coll2", coll2)

c = coll.get_id("c2")

#del c
#del coll
#del coll2
