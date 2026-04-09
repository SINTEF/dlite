"""Test dlite.table.Table"""
from pathlib import Path

import dlite
from dlite.dmtable import Table


thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"

# Not sure why, but this import seems to prevent the segfault
import test_storage


# Test loading a python table
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

# Test loading csv file
t2 = Table.from_csv(indir / "datamodels.csv")
m1, m2 = t2.get_datamodels()
assert isinstance(m1, dlite.Metadata)
assert isinstance(m2, dlite.Metadata)
assert m1.description == "First data model."
assert m1.getprop("length").type == "float64"
assert m1.getprop("length").unit == "cm"
assert m2.getprop("key").type == "string"
assert m2.getprop("indices").type == "int64"
assert m2.getprop("indices").shape.tolist() == ["N", "M"]

# Test loading excel file
t3 = Table.from_excel(indir / "datamodels.xlsx")
m3, m4 = t3.get_datamodels()
assert isinstance(m3, dlite.Metadata)
assert isinstance(m4, dlite.Metadata)
assert m3.description == "First data model."
assert m3.getprop("length").type == "float64"
assert m3.getprop("length").unit == "cm"
assert m4.getprop("key").type == "string"
assert m4.getprop("indices").type == "int64"
assert m4.getprop("indices").shape.tolist() == ["N", "M"]

# Test loading given sheet and cellrange from excel
t4 = Table.from_excel(
    indir / "datamodels.xlsx", sheet="sheet1", cellrange="A1:G2"
)
m5, = t4.get_datamodels()
assert isinstance(m5, dlite.Metadata)
assert m5.description == "First data model."
assert m5.getprop("length").type == "float64"
assert m5.getprop("length").unit == "cm"
