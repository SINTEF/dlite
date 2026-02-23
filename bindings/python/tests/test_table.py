"""Test dlite.table.Table"""
from dlite.table import Table


table = [
    ("identifier", "label", "description", "datumName[1]", "datumType[1]", "datumName[2]", "datumType[2]", "datumShape[2]"),
    ("dm1",        "dm1",   "...",         "mass",         "float64",      "symbol",       "string",       "len,nsymbols"),
    ("dm2",        "dm2",   "...",         "name",         "string",       None,             "",           ""),
]

t = Table(table, baseuri="http://onto-ns.com/meta/test/0.1/")
datamodels = t.get_datamodels()
print(datamodels[0])
