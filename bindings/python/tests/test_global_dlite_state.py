import dlite
from global_dlite_state_sub import assert_exists

uuids_in_istore=len(dlite.istore_get_uuids())

coll = dlite.Collection()
assert dlite.has_instance(coll.uuid)

assert_exists(coll.uuid)

assert coll.uuid in dlite.istore_get_uuids()
assert len(dlite.istore_get_uuids()) == uuids_in_istore+1