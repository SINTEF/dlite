"""Test dlite.table.Table"""
import dlite
from dlite.table import Table


table = [
    ("identifier", "label", "description", "datumName[1]", "datumType[1]", "datumName[2]", "datumType[2]", "datumShape[2]"),
    ("dm1",        "dm1",   "...",         "mass",         "float64",      "symbol",       "string",       "len,nsymbols"),
    ("dm2",        "dm2",   "...",         "name",         "string",       None,             "",           ""),
]

t = Table(table, baseuri="http://onto-ns.com/meta/test/0.1/")
dm1, dm2 = t.get_datamodels()

assert isinstance(dm1, dlite.Metadata)
assert isinstance(dm2, dlite.Metadata)
assert dm1.getprop("symbol").name == "symbol"
assert dm1.getprop("symbol").type == "string"
assert dm1.getprop("symbol").shape.tolist() == ["len", "nsymbols"]
