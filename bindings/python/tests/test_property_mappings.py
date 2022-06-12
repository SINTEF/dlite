#!/usr/bin/env python
from pathlib import Path

import dlite
import dlite.mappings as dm

try:
    import pint
except ImportError as exc:
    import sys
    print(f"Skipped: {exc}")
    sys.exit(44)  # exit code marking the test to be skipped


# Configure search paths
thisdir = Path(__file__).parent.absolute()
dlite.storage_path.append(f'{thisdir}/*.json')


mappings = [
    ('http://onto-ns.com/meta/0.1/Molecule#name', ':mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Molecule#groundstate_energy', ':mapsTo',
     'chem:GroundStateEnergy'),
    ('http://onto-ns.com/meta/0.1/Substance#id', ':mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Substance#molecule_energy', ':mapsTo',
     'chem:GroundStateEnergy'),
]


match = dm.match_factory(mappings)
match_first = dm.match_factory(mappings, match_first=True)



# Check unitconvert_pint
assert dm.unitconvert("km", 34, 'm') == 0.034
assert dm.unitconvert("Å", 34, 'um') == 34e4
assert dm.unitconvert("s", 1, 'hour') == 3600


# Test to manually set up mapping steps
v = dm.Value(3.0, 'm/s', 'emmo:Velocity', cost=1)
t = dm.Value(1.1, 's', 'emmo:Time', cost=2)
t2 = dm.Value(2.2, 's', 'emmo:Time', cost=4)
l = dm.Value(4.0, 's', 'emmo:Length', cost=8)

step1 = dm.MappingStep(
    output_iri='emmo:Length',
    steptype=dm.StepType.FUNCTION,
    function=lambda v, t: v*t,
    cost=lambda v, t: 2*v*t,
    output_unit='m',
)
step1.add_inputs({'v': v, 't': t})
step1.add_inputs({'v': v, 't': t2})

step2 = dm.MappingStep(
    output_iri=':Length',
    steptype=dm.StepType.MAPSTO,
    cost=2,
    output_unit='m',
)
step2.add_inputs({'l': step1})


step3 = dm.MappingStep(
    output_iri=':ReducedLength',
    steptype=dm.StepType.FUNCTION,
    function=lambda l: l - 1.0,
    cost=10,
    output_unit='m',
)
step3.add_inputs({'l': step1})
step3.add_inputs({'l': step2})
step3.add_inputs({'l': l})


def isclose(a, b, rtol=1e-3):
    """Returns true if the relative difference between `a` and `b` is less
    than `rtol`."""
    return True if abs((b - a)/b) <= rtol else False


assert isclose(3.0, step3.eval())
assert isclose(2.3, step3.eval(0))
assert isclose(5.6, step3.eval(1))
assert isclose(2.3, step3.eval(2))
assert isclose(5.6, step3.eval(3))
assert isclose(3.0, step3.eval(4))

costs = step3.lowest_costs(10)
assert len(costs) == 5
assert [idx for cost, idx in costs] == [4, 0, 2, 1, 3]
assert isclose(18.0, costs[0][0])
assert isclose(19.6, costs[1][0])
assert isclose(21.6, costs[2][0])
assert isclose(28.2, costs[3][0])
assert isclose(30.2, costs[4][0])





#routes = dm.mapping_route(
#    target='http://onto-ns.com/meta/0.1/Substance#molecule_energy',
#    sources=['http://onto-ns.com/meta/0.1/Molecule#groundstate_energy'],
#    triples=mappings)
