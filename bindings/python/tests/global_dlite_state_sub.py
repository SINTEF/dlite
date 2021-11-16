import dlite

def assert_exists(uuid):
    assert dlite.has_instance(uuid)
    assert uuid in dlite.istore_get_uuids()