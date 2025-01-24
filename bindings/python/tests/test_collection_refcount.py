"""A test that tries to reproduce what happens in oteapi-dlite."""
import dlite


def add_blob(coll):
    """Adds a blob instance to the collection."""
    Blob = dlite.get_instance("http://onto-ns.com/meta/0.1/Blob")
    blob = Blob([10])
    blob2 = Blob([2])
    coll.add("blob", blob)
    coll.add("blob2", blob2)


def use_blob(coll):
    """Access the blob instance in the collection."""
    blob = coll.get("blob")
    assert blob._refcount == 3  # coll, local blob, global blob


def use_blob2(coll):
    """Access the blob instance in the collection."""
    blob, blob2 = coll.get_instances()
    assert blob._refcount == 3  # coll, local blob, global blob
    assert blob2._refcount == 2  # coll, local blob2


coll = dlite.Collection()
assert coll._refcount == 1

add_blob(coll)
assert coll._refcount == 1

blob = coll.get("blob")
assert blob._refcount == 2  # coll, blob

use_blob(coll)
use_blob(coll)
assert coll._refcount == 1
assert blob._refcount == 2  # coll, blob

use_blob2(coll)
assert coll._refcount == 1
assert blob._refcount == 2  # coll, blob

use_blob2(coll)
assert blob._refcount == 2  # coll, blob


insts = list(coll.get_instances())
inst = insts[0]
assert inst._refcount == 3  # inst, blob, coll

del coll
assert inst._refcount == 2  # inst, blob

del blob
assert inst._refcount == 1  # inst
