import importlib
import dlite


def assert_exists_in_module(uuid):
    assert dlite.has_instance(uuid)
    assert uuid in dlite.istore_get_uuids()
