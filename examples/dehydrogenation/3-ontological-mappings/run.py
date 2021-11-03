#!/usr/bin/env python3
from typing import Dict, AnyStr
from pathlib import Path

import dlite


# Setup dlite paths
thisdir = Path(__file__).parent.absolute()
rootdir = thisdir.parent
workflow1dir = rootdir / '1-simple-workflow'
entitiesdir = rootdir / 'entities'
atomdata = workflow1dir / 'atomscaledata.json'
dlite.storage_path.append(f'{entitiesdir}/*.json')


mappings = [
    ('http://onto-ns.com/meta/0.1/Molecule#name', ':mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Molecule#energy', ':mapsTo',
     'chem:GroundStateEnergy'),
    ('http://onto-ns.com/meta/0.1/Substance#id', ':mapsTo',
     'chem:Identifier'),
    ('http://onto-ns.com/meta/0.1/Substance#molecule_energy', ':mapsTo',
     'chem:GroundStateEnergy'),
]

Molecule = dlite.get_instance('http://onto-ns.com/meta/0.1/Molecule')
Substance = dlite.get_instance('http://onto-ns.com/meta/0.1/Substance')


def create_match(triples):
    """Returns a match functions for `triples`."""
    def match(s=None, p=None, o=None):
        """Returns generator over all triples that matches (s, p, o)."""
        return (t for t in triples if
                (s is None or t[0] == s) and
                (p is None or t[1] == p) and
                (o is None or t[2] == o))
    return match

match = create_match(mappings)

def match_first(s=None, p=None, o=None):
    """Returns the first match.  If there are no matches, ``(None, None, None)``
    is returned."""
    return next(iter(match(s, p, o) or ()), (None, None, None))


mapsTo = 'mo:mapsTo'

for prop1 in Molecule['properties']:
    uri1 = f'{Molecule.uri}#{prop1.name}'
    for _, _, o1 in match(uri1, mapsTo, None):
        for prop2 in Substance['properties']:
            uri2 = f'{Substance.uri}#{prop2.name}'
            for _, _, o2 in match(uri2, mapsTo, o1):
                print(uri1, '==>', uri2)


class MappingError(Exception):
    """Base class for mapping errors."""

class InsufficientMappingError(MappingError):
    """There are properties or dimensions that are not mapped."""

class AmbiguousMappingError(MappingError):
    """A property maps to more than one value."""

class InconsistentDimensionError(MappingError):
    """The size of a dimension is assigned to more than one value."""


def unitconvert_pint(dest_unit, value, unit, conf=None):
    """Returns `value` converted to `dest_unit`.

    Args:
        dest_unit: Destination unit that `value` should be converted to.
        value: Source value.
        unit: The unit of the source value.
        conf: Optional configurations of the unit converter.
    """
    import pint
    ureg = pint.UnitRegistry()
    u1 = ureg(unit)
    u2 = ureg(dest_unit)
    return (value * u1).to(u2).m


def assign_dimensions(dims: Dict,
                      inst,
                      propname: AnyStr):
    """Assign dimensions from property assignment.

    Args:
        dims: A dict with (dimension name, dimension value) pairs that
          should be assigned.  Only values that are None will be assigned.
        inst: Source instance.
        propname: Source property name.
    """
    lst = [p.dims for p in inst.meta['properties'] if p.name == propname]
    if not lst:
        raise MappingError('Unexpected property name: {src_propname}')
    src_dims, = lst
    for dim in src_dims:
        if dim not in dims:
            raise InconsistentDimensionError('Unexpected dimension: {dim}')
        if dims[dim] is None:
            dims[dim] = inst.dimensions[dim]
        elif dims[dim] != inst.dimensions[dim]:
            raise InconsistentDimensionError(
                f'Trying to assign dimension {dim} of {src_inst.meta.uri} to '
                f'{src_inst.dimensions[dim]}, but it is already assigned to '
                f'{dims[dim]}')


def make_instance(meta, instances, mappings=(), strict=True,
                  allow_incomplete=False, unitconvert=unitconvert_pint,
                  mapsTo=':mapsTo'):
    """Create an instance of `meta` using data found in `*instances`.

    Args:
        meta: Metadata for the instance we will create.
        instances: sequence of instances that the new intance will be
            populated from.
        mappings: A sequence of triples defining the mappings.
        strict: If false, we will allow implicit mapping of properties
            with the same name.
        allow_incomplete: Whether to allow not populating all properties
            of the returned instance.
        unitconvert: A callable that converts between units.
        mapsTo: How the 'mapsTo' predicate is written in `mappings`.

    Returns:
        New instance of `meta` populated from `*instances`.

    Raises:
        InsufficientMappingError: There are properties or dimensions in the
          returned instance that are not mapped.
        AmbiguousMappingError: A property in the returned instance
          maps to more than one value.
        InconsistentDimensionError: The size of a dimension in the
          returned instance is assigned to more than one value.
    """
    match = create_match(mappings)  # match function

    if isinstance(instances, dlite.Instance):
        instances = [instances]

    dims = {d.name: None for d in meta['dimensions']}
    props = {}

    for prop in meta['properties']:
        prop_uri = f'{meta.uri}#{prop.name}'
        for _, _, o in match(prop_uri, mapsTo, None):
            for inst in instances:
                for prop2 in inst.meta['properties']:
                    prop2_uri = f'{inst.meta.uri}#{prop2.name}'
                    for _, _, o2 in match(prop2_uri, mapsTo, o):
                        value = inst[prop2.name]
                        if prop.name not in props:
                            assign_dimensions(dims, inst, prop2.name)
                            props[prop.name] = value
                        elif props[prop.name] != value:
                            raise AmbiguousMappingError(
                                f'"{prop.name}" maps to both '
                                f'"{props[prop.name]}" and "{value}"')

        if prop.name not in props and not strict:
            for inst in instances:
                if prop.name in inst.properties:
                    value = inst[prop.name]
                    if prop.name not in props:
                        assign_dimensions(dims, inst, prop.name)
                        props[prop.name] = value
                    elif props[prop.name] != value:
                        raise AmbiguousMappingError(
                            f'"{prop.name}" assigned to both '
                            f'"{props[prop.name]}" and "{value}"')

        if not allow_incomplete and prop.name not in props:
            raise InsufficientMappingError(
                f'no mapping for assigning property "{prop.name}" '
                f'in {meta.uri}')

    if None in dims:
        dimname = [k for k, v in dims.items() if v is None][0]
        raise InsufficientMappingError(f'dimension "{dimname}" is not assigned')

    inst = meta(list(dims.values()))
    for k, v in props.items():
        inst[k] = v
    return inst


def get_energy(reaction):
    """Calculates reaction energies with data from Substance entity
    data is harvested from collection and mapped to Substance according to
    mappings
    
    Args:
        reaction: dict with names of reactants and products ase keys
                  and stochiometric coefficient as value-
                  Negative stochiometric coefficients for reactants.
                  Positive stochiometric coefficients for products. 
    Returns:
        reaction energy
    """
    energy = 0
    for label, n in reaction.items():
        inst = make_instance(Substance, coll[label], mappings)
        energy+=n*inst.molecule_energy
    return energy




# Define where the molecule data is obtained from
# This is a dlite collection
coll = dlite.Collection(f'json://{atomdata}?mode=r#molecules', 0)


# input from chemical engineer, e.g. what are reactants and products
# reactants (left side of equation) have negative stochiometric coefficient
# products (right side of equation) have positive stochiometric coefficient
reaction1 = {'C2H6':-1, 'C2H4':1,'H2':1}

reaction_energy = get_energy(reaction1)
print('Reaction energy 1', reaction_energy)


reaction2 = {'C3H8':-1, 'H2': -2,'CH4':3}

reaction_energy2 = get_energy(reaction2)
print('Reaction energy 1', reaction_energy2)



# Map instance Molecule with label 'H2' to Substance
#inst = make_instance(Substance, coll['H2'], mappings)
#print(inst)

# Map instance Molecule with label 'H2' to itself
#inst2 = make_instance(Molecule, coll['H2'], mappings, strict=False)
#print(inst2)



