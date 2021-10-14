#!/usr/bin/env python3
import dlite
from optimade.adapters import Structure as OptimadeStructure
import requests

desired_optimade_structure = "https://optimade.odbx.science/structures/odbx/2"

raw_structure: dict = requests.get(desired_optimade_structure).json()

optimade_structure = OptimadeStructure(raw_structure.get("data", {}))

optimade_structure.nelements

spatial_dimensions = 3


Structure = dlite.Instance('json://OPTIMADEStructure.json')

struct = Structure((
    optimade_structure.nelements,
    optimade_structure.nsites,
    optimade_structure.nperiodic_dimensions,
    spatial_dimensions,
))

for prop in struct.properties:
    print(f"Setting property: {prop!r}")
    print(f"Actual value: {getattr(optimade_structure, prop, None)!r}")
    setattr(struct, prop, getattr(optimade_structure, prop, None))

print(struct)
