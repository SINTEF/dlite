# This file is executedevaluated by test_global_dlite_state.py
#
# Note that entitydir is defined in the global scope, so it should
# not be redefined here
from pathlib import Path

import dlite
from dlite import Instance, Dimension, Property, Relation

assert len(dlite.istore_get_uuids()) == 3 + 3


url = f"json://{entitydir}/MyEntity.json"

# myentity is already defined via test_global_dlite_state, no new instance is added to istore
myentity = Instance.from_url(url)
assert myentity.uri == "http://onto-ns.com/meta/0.1/MyEntity"
assert len(dlite.istore_get_uuids()) == 3 + 3

i1 = Instance.from_metaid(myentity.uri, [2, 3], "myid")
assert i1.uri == "myid"
assert i1.uuid in dlite.istore_get_uuids()
assert len(dlite.istore_get_uuids()) == 3 + 4
