"""Test dlite.table.DMTable"""
import warnings
from pathlib import Path

import dlite
from dlite.table import DMTable
from dlite.testutils import importcheck, raises
from dlite.dataset import MissingUnitError, UnknownUnitWarning


thisdir = Path(__file__).resolve().parent
indir = thisdir / "input"


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
assert dm11.ndimensions == 2
assert dm11.nproperties == 2
assert dm11.getprop("symbol").name == "symbol"
assert dm11.getprop("symbol").type == "string"
assert dm11.getprop("symbol").shape.tolist() == ["len", "nsymbols"]

assert isinstance(dm12, dlite.Metadata)
assert dm12.ndimensions == 0
assert dm12.nproperties == 1
assert dm12.getprop("name").type == "string"

assert isinstance(dm13, dlite.Metadata)
assert dm13.ndimensions == 0
assert dm13.nproperties == 2
assert dm13.getprop("scalar").type == "float64"
assert dm13.getprop("scalar").shape.tolist() == []


# Test loading csv file
t2 = DMTable.from_csv(indir / "datamodels.csv", unit_handling="force")
m21, m22 = t2.get_datamodels()
assert isinstance(m21, dlite.Metadata)
assert isinstance(m22, dlite.Metadata)
assert m21.description == "First data model."
assert m21.getprop("length").type == "float64"
assert m21.getprop("length").unit == "cm"
assert m22.getprop("key").type == "string"
assert m22.getprop("indices").type == "int64"
assert m22.getprop("indices").shape.tolist() == ["N", "M"]


# Test loading another csv file
t3 = DMTable.from_csv(indir / "datamodels2.csv")
# Line to be added in csv
# http://onto-ns.com/meta/test/0.1/m32,"Antother data model.","Datamodel 5.2",length,float64,cm,,indices,int,
#m31, m32 = t5.get_datamodels()
m31, = t3.get_datamodels()
assert isinstance(m31, dlite.Metadata)
assert m31.description == "First data model."
assert m31.ndimensions == 2
assert m31.nproperties == 2
assert m31.getprop("length").type == "float64"
assert m31.getprop("length").unit == "cm"
assert m31.getprop("length").shape.tolist() == ["N", "M"]
assert m31.getprop("indices").type == "int64"
assert m31.getprop("indices").shape.tolist() == ["N"]


# Test loading excel file
if importcheck("openpyxl"):
    t4 = DMTable.from_excel(indir / "datamodels.xlsx")
    m43, m44 = t4.get_datamodels()
    assert isinstance(m43, dlite.Metadata)
    assert isinstance(m44, dlite.Metadata)
    assert m43.description == "First data model."
    assert m43.getprop("length").type == "float64"
    assert m43.getprop("length").unit == "cm"
    assert m44.getprop("key").type == "string"
    assert m44.getprop("indices").type == "int64"
    assert m44.getprop("indices").shape.tolist() == ["N", "M"]

    # Test loading given sheet and cellrange from excel
    t5 = DMTable.from_excel(
        indir / "datamodels.xlsx", sheet="sheet1", cellrange="A1:G2"
    )
    m51, = t5.get_datamodels()
    assert isinstance(m51, dlite.Metadata)
    assert m51.description == "First data model."
    assert m51.getprop("length").type == "float64"
    assert m51.getprop("length").unit == "cm"


# Test saving to triplestore (slow...)
if importcheck("tripper") and importcheck("rdflib"):
    from tripper import Triplestore, EMMO, OWL, RDF, RDFS, SKOS
    from tripper.utils import en

    ts = Triplestore("rdflib")
    with warnings.catch_warnings():
        warnings.filterwarnings("ignore", "more than one unit with symbol.*", UserWarning)
        t2.to_triplestore(ts)

    #print(ts.serialize())
    m1 = "http://onto-ns.com/meta/test/0.1/m1"
    assert ts.has(m1, RDF.type, OWL.Class)
    assert ts.has(m1, RDFS.subClassOf, EMMO.Dataset)
    assert ts.has(m1, SKOS.prefLabel, en("M1"))

    length = "http://onto-ns.com/meta/test/0.1/m1#length"
    assert ts.has(length, RDF.type, OWL.Class)
    assert ts.has(length, RDFS.subClassOf, EMMO.Datum)
    assert ts.has(length, RDFS.subClassOf, EMMO.DoubleData)
    assert ts.has(length, SKOS.prefLabel, en("Length"))


# Test invalid unit
# Test loading a python table
table = [
    ("@id",     "@type",  "datumName[1]", "datumType[1]", "datumUnit[1]"),
    ("ex:dm61", "ex:DM1", "mass",         "float64",      "cm"),
    ("ex:dm62", "ex:DM1", "mass",         "float64",      "dansk mil"),
]
with raises(MissingUnitError):
    t6 = DMTable(table, baseuri="http://onto-ns.com/meta/test/0.2/")

with warnings.catch_warnings():
    warnings.filterwarnings("ignore", "dansk mil", UnknownUnitWarning)
    t6 = DMTable(
        table,
        baseuri="http://onto-ns.com/meta/test/0.2/",
        unit_handling="ignore"
    )
with dlite.HideDLiteWarnings():
    dm61, dm62 = t6.get_datamodels()
assert dm61.getprop("mass").unit == "cm"
assert not dm62.getprop("mass").unit

with warnings.catch_warnings():
    warnings.filterwarnings("ignore", "dansk mil", UnknownUnitWarning)
    t7 = DMTable(
        table,
        baseuri="http://onto-ns.com/meta/test/0.3/",
        unit_handling="force"
    )
with dlite.HideDLiteWarnings():
    dm71, dm72 = t7.get_datamodels()
assert dm71.getprop("mass").unit == "cm"
assert dm72.getprop("mass").unit == "dansk mil"
