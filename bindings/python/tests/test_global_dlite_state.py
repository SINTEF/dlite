import os
import importlib
import dlite
from dlite import Instance

from global_dlite_state_mod1 import assert_exists_in_module

thisdir = os.path.abspath(os.path.dirname(__file__))

assert len(dlite.istore_get_uuids()) == 3 # 3 Hardcoded dlite instances

coll = dlite.Collection() # (1)
assert dlite.has_instance(coll.uuid)
assert coll.uuid in dlite.istore_get_uuids()
assert len(dlite.istore_get_uuids()) == 3 + 1

# Must exist in imported dlite in different module (mod1)
assert_exists_in_module(coll.uuid)

url = 'json://' + thisdir + '/MyEntity.json' + "?mode=r"
e = Instance.create_from_url(url) # (2)
assert len(dlite.istore_get_uuids()) == 3 + 2

inst1 = Instance.create_from_metaid(e.uri, [3, 2])  # (3)
assert len(dlite.istore_get_uuids()) == 3 + 3

inst2 = Instance.create_from_metaid(e.uri, (3, 4), 'myinst')  # (4)
assert len(dlite.istore_get_uuids()) == 3 + 4

del inst1
assert len(dlite.istore_get_uuids()) == 3 + 3

# Use compile and exec with dlite defined in globals
env = globals().copy()
filename=os.path.join(thisdir, 'global_dlite_state_mod2.py')

with open(filename) as fd:
    exec(compile(fd.read(), filename, 'exec'), env)

# mod2 has added one instance
assert len(dlite.istore_get_uuids()) == 3 + 4

# Use importlib with mod3
importlib.import_module('global_dlite_state_mod3')

# mod3 has added one instance
assert len(dlite.istore_get_uuids()) == 3 + 5

importlib.__import__('global_dlite_state_mod4', globals=env, locals=None, fromlist=(), level=0)
# mod4 has added one instance
assert len(dlite.istore_get_uuids()) == 3 + 6
