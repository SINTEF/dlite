#!/usr/bin/env python
import sys
import importlib.util
from pathlib import Path

import numpy as np

try:
    import rdflib
    from pint import Quantity
    from tripper import Triplestore
except ImportError as exc:
    sys.exit(44)  # exit code marking the test to be skipped

import dlite
import tripper.mappings as tm


# Configure paths
thisdir = Path(__file__).parent.absolute()


# Test to manually set up mapping steps
v = tm.Value(3.0, "m/s", "emmo:Velocity", cost=1)
t = tm.Value(1.1, "s", "emmo:Time", cost=2)
t2 = tm.Value(2.2, "s", "emmo:Time", cost=4)
l = tm.Value(4.0, "m", "emmo:Length", cost=8)

step1 = tm.MappingStep(
    output_iri="emmo:Length",
    steptype=tm.StepType.FUNCTION,
    function=lambda v, t: v * t,
    cost=1,
    output_unit="m",
)
step1.add_inputs({"v": v, "t": t})
step1.add_inputs({"v": v, "t": t2})

step2 = tm.MappingStep(
    output_iri=":Length",
    steptype=tm.StepType.MAPSTO,
    cost=0.5,
    output_unit="m",
)
step2.add_inputs({"l": step1})


step3 = tm.MappingStep(
    output_iri=":ReducedLength",
    steptype=tm.StepType.FUNCTION,
    function=lambda l: 0.7 * l,
    cost=10,
    output_unit="m",
)
step3.add_inputs({"l": step1})
step3.add_inputs({"l": step2})
step3.add_inputs({"l": l})


def isclose(a, b, rtol=1e-3):
    """Returns true if the relative difference between `a` and `b` is less
    than `rtol`."""
    return True if abs((b - a) / b) <= rtol else False


assert step1.number_of_routes() == 2
assert step2.number_of_routes() == 2
assert step3.number_of_routes() == 5

assert isclose(Quantity(3 * 1.1, "m"), step1.eval(0))
assert isclose(Quantity(3 * 2.2, "m"), step1.eval(1))
assert isclose(Quantity(3 * 1.1, "m"), step1.eval())
assert isclose(Quantity(3 * 1.1, "m"), step2.eval(0))
assert isclose(Quantity(3 * 2.2, "m"), step2.eval(1))
assert isclose(Quantity(3 * 1.1, "m"), step2.eval())
assert isclose(Quantity(0.7 * 3 * 1.1, "m"), step3.eval(0))
assert isclose(Quantity(0.7 * 3 * 2.2, "m"), step3.eval(1))
assert isclose(Quantity(0.7 * 3 * 1.1, "m"), step3.eval(2))
assert isclose(Quantity(0.7 * 3 * 2.2, "m"), step3.eval(3))
assert isclose(Quantity(0.7 * 4.0, "m"), step3.eval(4))
assert isclose(Quantity(0.7 * 3 * 1.1, "m"), step3.eval())

costs = step3.lowest_costs(10)
assert len(costs) == 5
assert [idx for cost, idx in costs] == [0, 2, 1, 3, 4]
assert isclose(14.0, costs[0][0])
assert isclose(14.5, costs[1][0])
assert isclose(16.0, costs[2][0])
assert isclose(16.5, costs[3][0])
assert isclose(18.0, costs[4][0])


# ---------------------------------------
r = np.array([10, 20, 30, 40, 50, 60])  # particle radius [nm]
n = np.array([1, 3, 7, 6, 2, 1])  # particle number density [1e21 #/m^3]

rv = tm.Value(r, "nm", "inst1")
nv = tm.Value(n, "1/m^3", "inst2")


def average_radius(r, n):
    return np.sum(r * n) / np.sum(n)


mapsTo = "http://emmo.info/domain-mappings#mapsTo"
instanceOf = "http://emmo.info/datamodel#instanceOf"
subClassOf = "http://www.w3.org/2000/01/rdf-schema#subClassOf"
# description = 'http://purl.org/dc/terms/description'
label = "http://www.w3.org/2000/01/rdf-schema#label"
hasUnit = "http://emmo.info/datamodel#hasUnit"
hasCost = ":hasCost"
RDF = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
type = RDF + "type"
next = RDF + "next"
first = RDF + "first"
rest = RDF + "rest"
nil = RDF + "nil"
expects = "https://w3id.org/function/ontology#expects"
returns = "https://w3id.org/function/ontology#returns"


triples = [
    # Mappings for data models
    ("inst1", mapsTo, "mo:ParticleRadius"),
    ("inst2", mapsTo, "mo:NumberDensity"),
    ("inst3", mapsTo, "mo:AverageParticleRadius"),
    ("inst1", hasUnit, "um"),
    ("inst2", hasUnit, "1/m**3"),
    ("inst3", hasUnit, "um"),
    # Mappings for the function
    (":r", mapsTo, "mo:ParticleRadius"),
    (":n", mapsTo, "mo:NumberDensity"),
    (":ravg", mapsTo, "mo:AverageParticleRadius"),
    ("average_radius_function", type, "fno:Function"),
    ("average_radius_function", expects, "parameter_list"),
    ("average_radius_function", returns, "output_list"),
    ("parameter_list", type, "rdf:List"),
    ("parameter_list", first, ":r"),
    ("parameter_list", rest, "lst2"),
    ("lst2", type, "rdf:List"),
    ("lst2", first, ":n"),
    ("lst2", rest, nil),
    (":r", type, "fno:Parameter"),
    (":r", label, "r"),
    # (':r', hasUnit, 'um'),
    (":n", type, "fno:Parameter"),
    (":n", label, "n"),
    # (':n', hasUnit, '1/m**3'),
    ("output_list", type, "rdf:List"),
    ("output_list", first, ":ravg"),
    ("output_list", rest, nil),
    (":ravg", type, "fno:Output"),
    # (':ravg', hasUnit, 'm'),
]

ts2 = Triplestore("rdflib")
ts2.add_triples(triples)


step = tm.mapping_routes(
    target="inst3",
    sources={"inst1": r, "inst2": n},
    triplestore=ts2,
    function_repo={"average_radius_function": average_radius},
)

assert step.number_of_routes() == 1
assert step.lowest_costs() == [(22.0, 0)]
assert step.eval(unit="m") == 34e-6

print(step.show())
print("*** eval:", step.eval())
