"""A test that tries to reproduce what happens in oteapi-dlite."""
import dlite


def add_blob(coll):
    """Adds a blob instance to the collection."""
    Blob = dlite.get_instance("http://onto-ns.com/meta/0.1/Blob")
    blob = Blob([10])
    coll.add("blob", blob)
    return coll

def use_blob(coll):
    """Access the blob instance in the collection."""
    blob = coll.get("blob")
    assert blob._refcount == 2
    return coll

def use_blob2(coll):
    """Access the blob instance in the collection."""
    blob = next(coll.get_instances())
    assert blob._refcount == 2
    return coll


coll = dlite.Collection()
assert coll._refcount == 1
coll2 = add_blob(coll)
assert coll._refcount == 1
assert coll2._refcount == 1
use_blob(coll)
use_blob(coll)
use_blob2(coll)
assert coll._refcount == 1

blob = coll.get("blob")
assert blob._refcount == 3  # 2


insts = list(coll.get_instances())
inst = insts[0]
assert inst._refcount == 4  # 3

del coll, coll2
assert inst._refcount == 3  # 2

del blob
assert inst._refcount == 2  # 1
