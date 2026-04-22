"""Test dlite.table.DMTable"""
from pathlib import Path

import dlite
from dlite.table import DMTable
from dlite.testutils import importcheck


thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"

# Not sure why, but this import seems to prevent the segfault
import test_storage


# Test loading a python table
table = [
    ("@id", "label", "description", "datumName[1]", "datumType[1]", "datumName[2]", "datumType[2]", "datumShape[2]"),
    ("dm1", "dm1",   "...",         "mass",         "float64",      "symbol",       "string",       "len,nsymbols"),
    ("dm2", "dm2",   "...",         "name",         "string",       None,            "",            ""),
    ("dm3", "dm3",   "...",         "length",       "float",        "scalar",        "float64",     ""),
]
t1 = DMTable(table, baseuri="http://onto-ns.com/meta/test/0.1/")
dm11, dm12, dm13 = t1.get_datamodels()
assert isinstance(dm11, dlite.Metadata)
assert isinstance(dm12, dlite.Metadata)
assert dm11.getprop("symbol").name == "symbol"
assert dm11.getprop("symbol").type == "string"
assert dm11.getprop("symbol").shape.tolist() == ["len", "nsymbols"]

# Test loading csv file
t2 = DMTable.from_csv(indir / "datamodels.csv")
m21, m22 = t2.get_datamodels()
assert isinstance(m21, dlite.Metadata)
assert isinstance(m22, dlite.Metadata)
assert m21.description == "First data model."
assert m21.getprop("length").type == "float64"
assert m21.getprop("length").unit == "cm"
assert m22.getprop("key").type == "string"
assert m22.getprop("indices").type == "int64"
assert m22.getprop("indices").shape.tolist() == ["N", "M"]

# Test loading excel file
if importcheck("openpyxl"):
    t3 = DMTable.from_excel(indir / "datamodels.xlsx")
    m33, m34 = t3.get_datamodels()
    assert isinstance(m33, dlite.Metadata)
    assert isinstance(m34, dlite.Metadata)
    assert m33.description == "First data model."
    assert m33.getprop("length").type == "float64"
    assert m33.getprop("length").unit == "cm"
    assert m34.getprop("key").type == "string"
    assert m34.getprop("indices").type == "int64"
    assert m34.getprop("indices").shape.tolist() == ["N", "M"]

    # Test loading given sheet and cellrange from excel
    t4 = DMTable.from_excel(
        indir / "datamodels.xlsx", sheet="sheet1", cellrange="A1:G2"
    )
    m41, = t4.get_datamodels()
    assert isinstance(m41, dlite.Metadata)
    assert m41.description == "First data model."
    assert m41.getprop("length").type == "float64"
    assert m41.getprop("length").unit == "cm"

# Test loading another csv file
t5 = DMTable.from_csv(indir / "datamodels2.csv")
# Line to be added in csv
# http://onto-ns.com/meta/test/0.1/m52,"Antother data model.","Datamodel 5.2",length,float64,cm,,indices,int,
#m51, m52 = t5.get_datamodels()
m51, = t5.get_datamodels()
assert isinstance(m51, dlite.Metadata)
assert m51.description == "First data model."
assert m51.ndimensions == 2
assert m51.nproperties == 2
assert m51.getprop("length").type == "float64"
assert m51.getprop("length").unit == "cm"
assert m51.getprop("length").shape.tolist() == ["N", "M"]
assert m51.getprop("indices").type == "int64"
assert m51.getprop("indices").shape.tolist() == ["N"]
