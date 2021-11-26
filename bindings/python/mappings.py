"""Implements mappings between entities.
"""
from __future__ import annotations

import types
import itertools
import warnings
from collections import defaultdict
from collections.abc import Sequence
from typing import Union, Dict, List

import dlite


class MappingError(Exception):
    """Base class for mapping errors."""


class InsufficientMappingError(MappingError):
    """There are properties or dimensions that are not mapped."""


class AmbiguousMappingError(MappingError):
    """A property maps to more than one value."""


class InconsistentDimensionError(MappingError):
    """The size of a dimension is assigned to more than one value."""


class InconsistentTriplesError(MappingError):
    """Inconsistcy in RDF triples."""


class MappingStep:
    """A step in a mapping route from a target to one or more source properties.

    A mapping step corresponds to one or more RDF triples.  In the
    simple case of a `:mapsTo` or `rdfs:isSubclassOf` relation, it is
    only one triple.  For transformations that has several input and
    output, a set of triples are expected.

    Subproperty relations should be resolved using the ontology before
    creating a mapping step.

    Args:
        inputs: Sequence of inputs to this mapping step.  Should either be
          MappingStep objects for the peceeding mapping steps or the URI
          of source property.
        output: IRI of the output concept.
        predicate: Either "mapsTo", "subClassOf" or "hasInput", corresponding
          to a mapping, inference or transformation step.
        triples: Optional. Sequence of RDF triples for this step.  Might be
          used for visualisation.
        unit: The output of this step.
        config: Configuration of a transformation step.

    Attributes:
        inputs: Sequence of inputs to this mapping step.  Should either be
          MappingStep objects for the peceeding mapping steps or the URI
          of source property.
        predicate: Either "mapsTo", "subClassOf" or "hasInput", corresponding
          to a mapping, inference or transformation step.
        output: IRI of the output concept.
        triples: Copy of the `triples` argument.
        unit: The output of this step.
        config: Configuration of a transformation step.
        next: List with the next mapping step.
    """
    def __init__(self,
                 inputs: Sequence[Union[str, MappingStep]],
                 output: str,
                 predicate: Union['mapsTo', 'subClassOf', 'hasInput'] = 'mapsTo',
                 triples: Sequence[(str, str, str)] = (),
                 unit: str = None,
                 config: dict = None,
                 ) -> None:
        self.inputs = []
        self.predicate = predicate
        self.output = output
        self.triples = [(t[0], t[1], t[2]) for t in triples]
        self.unit = unit
        self.config = config
        for input in inputs:
            self.add_input(input)
        self.next = []

    def add_input(self, step: MappingStep) -> None:
        """Adds an input mapping step."""
        self.inputs.append(input)
        if isinstance(input, MappingStep):
            input.next.append(self)

    def get_sources(self) -> List[str]:
        """Returns a list of URIs of all source metadata."""
        sources = []
        if not self.inputs:
            warnings.warn(f'')
        for input in self.inputs:
            if isinstance(input, MappingStep):
                sources.extend(input.get_sources())
            else:
                sources.append(input)
        return sources

    def eval(self, input_values, input_units=None, unit=None):
        """Returns the evaluated value."""
        pass


def match_factory(triples, match_first=False):
    """A factory function that returns a match functions for `triples`.

    If `match_first` is false, the returned match function will return
    a generator over all matching triples.  Otherwise the the returned
    function will only return the first matching triple.

    Args:
        triples: A list of triples.
        match_first: Whether the returned match function should return
          only the first match.

    Example:
    >>> triples = [
    ...     (':mamal', 'rdfs:subClassOf', ':animal'),
    ...     (':cat', 'rdfs:subClassOf', ':mamal'),
    ...     (':mouse', 'rdfs:subClassOf', ':mamal'),
    ...     (':cat', ':eats', ':mouse'),
    ... ]
    >>> match = match_factory(triples)
    >>> match_first = match_factory(triples, only_first=True)
    >>> list(match(':cat'))
    [(':cat', 'rdfs:subClassOf', ':mamal'),
     (':cat', ':eats', ':mouse')]
    >>> match_first(':cat', None, None)
    (':cat', 'rdfs:subClassOf', ':mamal')
    """
    def match(s=None, p=None, o=None):
        """Returns generator over all triples that matches (s, p, o)."""
        return (t for t in triples if
                (s is None or t[0] == s) and
                (p is None or t[1] == p) and
                (o is None or t[2] == o))
    if match_first:
        return lambda s=None, p=None, o=None: next(
            iter(match(s, p, o) or ()), (None, None, None))
    else:
        return match


def mapping_route(
        target, sources, triples,
        mapsTo=':mapsTo',
        subClassOf='http://www.w3.org/2000/01/rdf-schema#subClassOf',
        subPropertyOf='http://www.w3.org/2000/01/rdf-schema#subPropertyOf',
        hasInput=':hasInput',
        hasOutput=':hasOutput'):
    """Finds the route of mappings from any source in `sources` to `target`.

    This implementation takes transitivity, subclasses and
    subproperties into accaount.

    Args:
        target: IRI of the target in `triples`.
        sources: Sequence of source IRIs in `triples`.
        triples: Sequence of (subject, predicate, object) triples.
          It is safe to pass a generator expression too.
        mapsTo: How 'mapsTo' is written in `triples`.
        subClassOf: How 'subClassOf' is written in `triples`.  Set it
          to None if subclasses should not be considered.
        subPropertyOf: How 'subPropertyOf' is written in `triples`.  Set it
          to None if subproperties of `mapsTo` should not be considered.
        hasInput: How 'hasInput' is written in `triples`.
        hasOutput: How 'hasOutput' is written in `triples`.

    Returns:
        list: Names of all sources that maps to `target`.

        list: A nested list with different mapping routes from `target`
          to a source, where a mapping route is expressed as a
          list of triples.  For example:

              [(target, mapsTo, 'onto:A'),
               ('onto:A', mapsTo, 'onto:B'),
               (source1, mapsTo, 'onto:B')]

    Bugs:
        In the current implementation will the returned mapping route
        report sub properties of `mapsTo` as `mapsTo`.  Some
        postprocessing is required to fix this.
    """
    sources = set(sources)

    # Create a set of 'relations' to consider, consisting of mapsTo and
    # its sub properties
    if subPropertyOf:
        def walk(src, d):
            yield src
            for s in d[src]:
                yield from walk(s, d)

        def get_relations(rel):
            """Returns a set of `rel` and its subproperties."""
            oSPs = defaultdict(set)
            for s, p, o in triples:
                if p == subPropertyOf:
                    oSPs[o].add(s)
            return set(walk(rel, oSPs))

        if isinstance(triples, types.GeneratorType):
            # Convert generator to a list such that we can transverse it twice
            triples = list(triples)

        #oSPs = defaultdict(set)  # (o, subPropertyOf, s) ==> oSPs[o] -> {s, ..}
        #for s, p, o in triples:
        #    if p == subPropertyOf:
        #        oSPs[o].add(s)
        #relations = set(walk(mapsTo, oSPs))
        #del oSPs
        relations = get_relations(mapsTo)
    else:
        relations = set([mapsTo])

    # Create lookup tables for fast access to properties
    # This only transverse `tiples` once
    sRo = defaultdict(list)   # (s, mapsTo, o)     ==> sRo[s] -> [o, ..]
    oRs = defaultdict(list)   # (o, mapsTo, s)     ==> oRs[o] -> [s, ..]
    sSCo = defaultdict(list)  # (s, subClassOf, o) ==> sSCo[s] -> [o, ..]
    oSCs = defaultdict(list)  # (o, subClassOf, s) ==> oSCs[o] -> [s, ..]
    for s, p, o in triples:
        if p in relations:
            sRo[s].append(o)
            oRs[o].append(s)
        elif p == subClassOf:
            sSCo[s].append(o)
            oSCs[o].append(s)

    # The lists to return, populated with walk_forward() and walk_backward()
    mapped_sources = []
    mapped_routes = []

    def walk_forward(entity, visited, route):
        """Walk forward from `entity` in the direction of mapsTo."""
        if entity not in visited:
            walk_backward(entity, visited, route)
            for e in sRo[entity]:
                walk_forward(
                    e, visited.union(set([entity])),
                    route + [(entity, mapsTo, e)])
            for e in oSCs[entity]:
                walk_forward(
                    e, visited.union(set([entity])),
                    route + [(e, subClassOf, entity)])

    def walk_backward(entity, visited, route):
        """Walk backward from `entity` to a source, against the direction of
        mapsTo."""
        if entity not in visited:
            if entity in sources:
                mapped_sources.append(entity)
                mapped_routes.append(route)
            else:
                for e in oRs[entity]:
                    walk_backward(
                        e, visited.union(set([entity])),
                        route + [(e, mapsTo, entity)])
                for e in sSCo[entity]:
                    walk_backward(
                        e, visited.union(set([entity])),
                        route + [(entity, subClassOf, e)])

    walk_forward(target, set(), [])

    return mapped_sources, mapped_routes



#
# def mapping_targets(source, triples, mapsTo=':mapsTo', return_routes=False):
#     """Finds all targets that `source` maps to.
#
#     This implementation takes the transitivity of mapsTo into accaount.
#
#     Args:
#         source: IRI of source in `triples`.
#         triples: Sequence of (subject, predicate, object) triples.
#         mapsTo: How the 'mapsTo' predicate is written in `triples`.
#         return_routes: Whether to also return a list of tuples showing the
#           mapping route.
#
#     Returns:
#         list: Name of all targets that `source` maps to.
#         list: (optional) If `return_route` is true, a list of mapping routes
#           corresponding to the list of targets is also returned.
#           Each route is expressed as a list of triples.  For example
#           can the route from 'a' to 'b' be expressed as
#
#               [[('a', ':mapsTo', 'onto:A'), ('b', ':mapsTo', 'onto:A')]]
#     """
#     import itertools
#     match = match_factory(triples)  # match function
#
#     # Trivial implementation
#     #for _, _, cls in match(s=source, p=mapsTo):
#     #    for target, _, _ in match(p=mapsTo, o=cls):
#     #        targets.append(target)
#     #        if return_routes:
#     #            routes.append([(source, mapsTo, cls), (target, mapsTo, cls)])
#
#     # Recursive implementation taking transitivity into account
#     targets = []
#     routes = []
#
#     def add_target(target, route):
#         targets.append(target)
#         if return_routes:
#             routes.append(route)
#
#     def find_target(cls, route, visited):
#         """Find all targets that maps to `cls`.
#         Returns true if cls correspond to a final target, otherwise false."""
#         m = match(p=mapsTo, o=cls)
#         try:
#             s, p, o = m.__next__()
#         except StopIteration:
#             return True
#
#         # Use itertools.chain() to put (s, p, o) in what we are iterating over
#         for s, p, o in itertools.chain(iter([(s, p, o)]), m):
#             if s in visited:
#                 pass
#             else:
#                 if find_target(s, route + [s, p, o], visited + [s]):
#                     add_target(s, route + [s, p, o])
#         return False
#
#     def find(s, route, visited):
#         """Find all classes that `s` maps to."""
#         for s, p, o in match(s=s, p=mapsTo):
#             find_target(o, route + [(s, p, o)], visited + [s])
#             find(o, route + [(s, p, o)], visited + [s])
#
#     find(source, [], [])
#
#     if return_routes:
#         return targets, routes
#     else:
#         return targets
#

def unitconvert_pint(dest_unit, value, unit):
    """Returns `value` converted to `dest_unit`.

    A unitconvert function based on Pint.  Alternative functions
    based on ontologies may be implemented.

    Args:
        dest_unit: Destination unit that `value` should be converted to.
        value: Source value.
        unit: The unit of the source value.
    """
    import pint
    ureg = pint.UnitRegistry()
    u1 = ureg(unit)
    u2 = ureg(dest_unit)
    return (value * u1).to(u2).m


def assign_dimensions(dims: Dict,
                      inst: dlite.Instance,
                      propname: str):
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
                f'Trying to assign dimension {dim} of {src_inst.meta.uri} '
                f'to {src_inst.dimensions[dim]}, but it is already assigned '
                f'to {dims[dim]}')


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
        unitconvert: A callable that converts between units.  It has
          prototype

              unitconvert(dest_unit, value, unit)

          and should return `value` (in units `unit`) converted to
          `dest_unit`.
        mapsTo: How the 'mapsTo' predicate is written in `mappings`.

    Returns:
        New instance of `meta` populated from `*instances`.

    Raises:
        InsufficientMappingError: There are properties or dimensions in
          the returned instance that are not mapped.
        AmbiguousMappingError: A property in the returned instance
          maps to more than one value.
        InconsistentDimensionError: The size of a dimension in the
          returned instance is assigned to more than one value.

    Todo:
    - Consider that mapsTo is a transitive relation.
    - Use EMMOntoPy to also account for rdfs:subClassOf relations.
    - Consider the possibility to assign values via the `mappings`
      triples.  Do we really want that?  May be useful, but will add
      a lot of complexity.  Should we have different relations for
      default values and values that will overwrite what is provided
      from a matching input instance?
    - Add `mapsToPythonExpression` subproperty of `mapsTo` to allow
      writing analytical Python expression for a property as a function
      of properties defined in the ontology.
      Use the ast module for safe evaluation to ensure that this feature
      cannot be misused for code injection.
    - Add a function that visualise the possible mapping paths.
    """
    match = match_factory(mappings)  # match function

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
        raise InsufficientMappingError(
            f'dimension "{dimname}" is not assigned')

    inst = meta(list(dims.values()))
    for k, v in props.items():
        inst[k] = v
    return inst
