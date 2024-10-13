"""Test storages."""
from pathlib import Path

import numpy as np

import dlite
from dlite.testutils import importcheck, raises
from dlite.protocol import Protocol, archive_extract

yaml = importcheck("yaml")
requests = importcheck("requests")


thisdir = Path(__file__).absolute().parent
outdir = thisdir / "output"
indir = thisdir / "input"
entitydir = thisdir / "entities"
plugindir = thisdir / "plugins"
dlite.storage_path.append(entitydir / "*.json")
dlite.python_storage_plugin_path.append(plugindir)


# Test plugin that only defines to_bytes() and from_bytes()
dlite.Storage.plugin_help("bufftest")
inst = dlite.Instance.from_metaid(
    "http://onto-ns.com/meta/0.1/Blob", dimensions=[3]
)
inst.content = b"abc"
buf = inst.to_bytes("bufftest")

# Change ID in binary representation before creating a new instance
buf2 = bytearray(buf)
buf2[5:9] = b'0123'
inst2 = dlite.Instance.from_bytes("bufftest", buf2)
assert inst2.uuid != inst.uuid
assert inst2.dimensions == inst.dimensions
assert all(inst2.content == inst.content)
