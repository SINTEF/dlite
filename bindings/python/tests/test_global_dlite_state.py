import dlite
from global_dlite_state_sub import assert_exists

uuids_in_istore=len(dlite.istore_get_uuids())

# When this test_global_dlite_state.py is run standalone, we have uuids_in_istore == 3 (Basic, hardcoded dlite instances,
# since we have not added anything yet)

# But when this test is run as a testcase in test_python_bindings.py, this test is executed in the same Python process
# as previous tests and those previous tests have already populated dlites internal storage (which nicely illustrates the point).

if uuids_in_istore in [3, 12]:
    pass
else:
    msg="\n"
    for n, uuid in enumerate(dlite.istore_get_uuids()):
        inst=dlite.get_instance(uuid)
        msg+=f"{n}: {uuid}, {inst.uri}\n---\n {inst}\n"
    raise RuntimeError(msg)

coll = dlite.Collection()
assert dlite.has_instance(coll.uuid)

assert_exists(coll.uuid)

assert coll.uuid in dlite.istore_get_uuids()
assert len(dlite.istore_get_uuids()) == uuids_in_istore+1