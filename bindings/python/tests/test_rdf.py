from pathlib import Path

import dlite
from dlite.rdf import to_rdf, from_rdf

try:
    import rdflib
except ImportError:
    import sys
    sys.exit(44)


thisdir = Path(__file__).resolve().parent

id = "http://onto-ns.com/data#my_test_instance"
inst = from_rdf(thisdir / "rdf.ttl", id=id)

# Serialise `inst` to string `s`
s = to_rdf(inst, base_uri="http://onto-ns.com/data#",
           base_prefix="onto", include_meta=True)

# Check that content matches original serialisation
with open(thisdir / "rdf.ttl", "r") as f:
    orig = f.read()
assert s == orig
