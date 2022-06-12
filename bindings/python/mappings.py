"""Implements mappings between entities.
"""
from __future__ import annotations

import itertools
import types
import warnings
from collections import defaultdict
from collections.abc import Sequence
from enum import Enum
from typing import Any, Callable, Dict, List, Optional, Union

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


class MissingRelationError(MappingError):
    """There are missing relations in RDF triples."""


class StepType(Enum):
    """Type of mapping step when going from the output to the inputs."""
    MAPSTO = 1
    INV_MAPSTO = -1
    INSTANCEOF = 2
    INV_INSTANCEOF = -2
    SUBCLASSOF = 3
    INV_SUBCLASSOF = -3
    FUNCTION = 4


class Value:
    """Represents the value of an instance property.

    Arguments:
        value: Property value.
        unit: Property unit.
        iri: IRI of ontological concept that this value is an instance of.
        property_iri: IRI of datamodel property that this value is an
            instance of.
        cost: Cost of accessing this value.
    """
    def __init__(self, value, unit=None, iri=None, property_iri=None, cost=0.0):
        self.value = value
        self.unit = unit
        self.iri = iri
        self.property_iri = property_iri
        self.cost = cost


class MappingStep:
    """A step in a mapping route from a target to one or more sources.

    A mapping step corresponds to one or more RDF triples.  In the
    simple case of a `mo:mapsTo` or `rdfs:isSubclassOf` relation, it is
    only one triple.  For transformations that has several input and
    output, a set of triples are expected.

    Arguments:
        output_iri: IRI of the output concept.
        steptype: One of the step types from the StepType enum.
        function: Callable that evaluates the output from the input.
        cost: The cost related to this mapping step.  Should be either a
            float or a callable taking the same arguments as `function` as
            input returning the cost as a float.
        input_units: Dict mapping input names to expected units.
        output_unit: Output unit.

    The arguments can also be assigned as attributes.
    """
    def __init__(
            self,
            output_iri: Optional[str] = None,
            steptype: Optional[StepType] = None,
            function: Optional[Callable] = None,
            cost: Union[Any, Callable] = 1.0,
            input_units: Optional[dict] = None,
            output_unit: Optional[str] = None,
    ):
        self.output_iri = output_iri
        self.steptype = steptype
        self.function = function
        self.cost = cost
        self.input_units = input_units if input_units else {}
        self.output_unit = output_unit
        self.input_routes = []  # list of inputs dicts
        self.join_mode = False  # whether to join upcoming input
        self.joined_input = {}

    def add_inputs(self, inputs):
        """Add input dict for an input route."""
        self.input_routes.append(inputs)

    def add_input(self, input, name=None):
        """Add input."""
        assert isinstance(input, (MappingStep, Value))
        argname = name if name else f'arg{len(self.joined_input)+1}'
        if self.join_mode:
            joined_input[argname] = input
        else:
            self.add_inputs({argname: input})

    def join_input(self):
        """Join all input added with add_input() since `join_mode` was set true.
        Resets `join_mode` to false."""
        if not self.join_mode:
            raise MappingError('Calling join_input() when join_mode is false.')
        self.join_mode = False
        self.add_inputs(self.joined_input)
        self.joined_input.clear()

    def eval(self, routeno=None):
        """Returns the evaluated value of given input route number.

        If `routeno` is None (default) the route with the lowest cost
        is evalueated."""
        if routeno is None:
            (_, routeno), = self.lowest_costs(nresults=1)
        inputs, idx = self.get_inputs(routeno)
        values = get_values(inputs, idx, self.input_units)
        if self.function:
            return self.function(**values)
        elif len(values) == 1:
            value, = values.values()
            return value
        else:
            raise TypeError("Expected inputs to be a single argument")

    def get_inputs(self, routeno):
        """Returns input and input index `(inputs, idx)` for route number
        `routeno`."""
        n = 0
        for inputs in self.input_routes:
            n0 = n
            n += get_nroutes(inputs)
            if n > routeno:
                return inputs, routeno - n0
        raise ValueError(f"routeno={routeno} exceeds number of routes")

    def get_input_iris(self, routeno):
        """Returns a dict mapping input names to iris for the given route
        number."""
        inputs, idx = self.get_inputs(routeno)
        return {k: v.output_iri if isinstance(v, MappingStep) else v.iri
                for k, v in inputs.items()}

    def number_of_routes(self):
        """Returns total number of routes to this mapping step."""
        n = 0
        for inputs in self.input_routes:
            n += get_nroutes(inputs)
        return n

    def lowest_costs(self, nresults=5):
        """Returns a list of `(cost, routeno)` tuples with up to the `nresult`
        lowest costs and their corresponding route numbers."""
        result = []
        n = 0
        for inputs in self.input_routes:
            owncost = 1
            for cost, idx in get_lowest_costs(inputs, nresults=nresults):
                if isinstance(self.cost, Callable):
                    values = get_values(inputs, idx, self.input_units)
                    owncost = self.cost(**values)
                else:
                    owncost = self.cost
                result.append((cost + owncost, n + idx))
            n += get_nroutes(inputs)
        return sorted(result)[:nresults]


def get_nroutes(inputs):
    """Help function returning the number of routes for an input dict."""
    m = 1
    for input in inputs.values():
        if isinstance(input, MappingStep):
            m *= input.number_of_routes()
    return m


def get_values(inputs, idx, input_units):
    """Help function returning a dict mapping the input names to actual value
    of expected input unit.

    There exists `get_nroutes(inputs)` routes to populate `inputs`.
    `idx` is the index of the specific route we will use to obtain the
    values."""
    values = {}
    for k, v in inputs.items():
        if isinstance(v, MappingStep):
            value, unit = v.eval(idx), v.output_unit
        else:
            value, unit = v.value, v.unit

        if unit and k in input_units:
            values[k] = unitconvert(input_units[k], value, unit)
        else:
            values[k] = value
    return values


def get_lowest_costs(inputs, nresults=5):
    """Returns a list of `(cost, routeno)` tuples with up to the `n`
    lowest costs and their corresponding route numbers."""
    result = []
    vcost = 0
    for input in inputs.values():
        if isinstance(input, MappingStep):
            result.extend(input.lowest_costs(nresults=nresults))
        else:
            vcost += input.cost
    if result:
        result.sort()
        result = [(cost + vcost, idx) for cost, idx in result[:nresults]]
    else:
        result.append((vcost, 0))
    return result


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


def fno_mapper(triples):
    """Finds all function definitions in `triples` based on the function
    ontoloby (FNO).

    Sweep through triples and return a dict mapping output IRIs to a list
    of `(function_iri, [input_iris, ...])` tuples.
    """
    expects = 'https://w3id.org/function/ontology#expects'
    returns = 'https://w3id.org/function/ontology#returns'
    first = 'http://www.w3.org/1999/02/22-rdf-syntax-ns#first'
    rest = 'http://www.w3.org/1999/02/22-rdf-syntax-ns#rest'

    # Temporary dicts for fast lookup
    Dexpects = defaultdict(list)
    Dreturns = defaultdict(list)
    Dfirst = defaultdict(list)
    Drest = defaultdict(list)
    for s, p, o in triples:
        if p == expects:
            Dexpects[s].append(o)
        elif p == returns:
            Dreturns[s].append(o)
        elif p == first:
            Dfirst[s].append(o)
        elif p == rest:
            Drest[s].append(o)

    d = defaultdict(list)
    for func, lst in Dreturns.items():
        input_iris = []
        for exp in Dexpects.get(func, ()):
            if exp in Dfirst:
                while exp in Dfirst:
                    input_iris.append(Dfirst[exp])
                    if exp not in Drest:
                        break
                    exp = Drest[exp]
            else:
                # Support also misuse of FNO, where fno:expects refers
                # directly to input individuals
                input_iris.append(exp)

        for ret in lst:
            if ret in Dfirst:
                while ret in Dfirst:
                    d[Dfirst[ret]].append((func, input_iris))
                    if ret not in Drest:
                        break
                    ret = Drest[ret]
            else:
                # Support also misuse of FNO, where fno:returns refers
                # directly to the returned individual
                d[ret].append((func, input_iris))

    return d


def mapping_route(
        target, sources, triples,
        function_repo=None,
        function_mappers=(fno_mapper, ),
        default_costs={'function': 10.0, 'mapsTo': 2.0, 'instanceOf': 1.0,
                       'subClassOf': 1.0, 'value': 0.0},
        mapsTo='http://emmo.info/domain-mappings#mapsTo',
        instanceOf='http://emmo.info/datamodel#instanceOf',
        subClassOf='http://www.w3.org/2000/01/rdf-schema#subClassOf',
        #type='http://www.w3.org/1999/02/22-rdf-syntax-ns#type',
        #description='http://purl.org/dc/terms/description',
        hasName='http://www.w3.org/2000/01/rdf-schema#label',
        hasUnit='http://emmo.info/datamodel#hasUnit',
        hasCost=':hasCost',
        hasValue='http://emmo.info/datamodel#hasValue',
):
    """Find routes of mappings from any source in `sources` to `target`.

    This implementation supports transitivity, subclasses.

    Arguments:
        target: IRI of the target in `triples`.
        sources: Sequence of source IRIs in `triples`.
        triples: Sequence of (subject, steptype, object) triples.
            It is safe to pass a generator expression too.
        mapsTo: How 'mapsTo' is written in `triples`.
        subClassOf: How 'subClassOf' is written in `triples`.  Set it
            to None if subclasses should not be considered.
        function_mappers: Sequence of mapping functions that takes `triples`
            as argument and return a dict mapping output IRIs to a list
            of `(function_iri, [input_iris, ...])` tuples.
        hasName: How 'hasName' is written in `triples`.  Used for naming
            function in put parameters.  The default is to use rdfs:label.
        hasUnit: How 'hasUnit' is written in `triples`.
        hasCost: How 'hasCost' is written in `triples`.
        hasValue: How 'hasValue' is written in `triples`.

    Returns:
        A nested graph of MappingStep instances.
    """
    sources = set(sources)

    # We need to parse triples more than once, so create a local list
    # if it is a generator or in iterator
    if hasattr(triples, '__next__'):
        triples = list(triples)

    # Create lookup tables for fast access to properties
    # This only transverse `tiples` once
    soMaps  = defaultdict(list)  # (s, mapsTo, o)     ==> soMaps[s] -> [o, ..]
    osMaps  = defaultdict(list)  # (o, mapsTo, s)     ==> osMaps[o] -> [s, ..]
    #soSubcl = defaultdict(list)  # (s, subClassOf, o) ==> soSubcl[s] -> [o, ..]
    osSubcl = defaultdict(list)  # (o, subClassOf, s) ==> osSubcl[o] -> [s, ..]
    soInst  = dict()             # (s, instanceOf, o) ==> soInst[s] -> o
    osInst  = defaultdict(list)  # (o, instanceOf, s) ==> osInst[o] -> [s, ..]
    #soType  = defaultdict(list)  # (o, type, s)       ==> soType[s] -> [o, ..]
    soName  = list()             # (o, hasName, s)    ==> soName[s] -> o
    soUnit  = dict()             # (s, hasUnit, o)    ==> soUnit[s] -> o
    soCost  = dict()             # (s, hasCost, o)    ==> soCost[s] -> o
    soValue = dict()             # (s, hasValue, o)   ==> soValue[s] -> o
    for s, p, o in triples:
        if p == mapsTo:
            soMaps[s].append(o)
            osMaps[o].append(s)
        elif p == subClassOf:
            #soSubcl[s].append(o)
            osSubcl[o].append(s)
        #elif p == type:
        #    soType[s].append(o)
        elif p == instanceOf:
            if s in soInst:
                raise InconsistentTriplesError(
                    f'The same individual can only relate to one datamodel '
                    f'property via {instanceOf} relations.')
            soInst[s] = o
            osInst[o].append(s)
        elif p == hasUnit:
            soUnit[s] = o
        elif p == hasCost:
            soCost[s] = o
        elif p == hasValue:
            soValue[s] = o

    # Function dicts
    fdicts = [fmapper(triples) in function_mappers]

    def walk(target, visited, step):
        """Walk backward in rdf graph from `node` to sources."""
        if target in visited: return
        visited.add(target)

        if target in sources:
            if target not in soValue:
                raise MissingRelationError(
                    f'Missing "hasValue" relation on: {target}')
            value = Value(value=soValue[target], unit=soUnit.get(target),
                          iri=target, property_iri=soInst.get(target),
                          cost=soCost.get(target, default_cost['value']))
            step.add_input(value, name=soName.get(target))
            return

        for node in osInst[target]:
            step0 = MappingStep(output_iri=target,
                                output_unit=soUnit.get(target))
            step0.steptype = StepType.INV_INSTANCEOF
            step0.cost = soCost.get(target, default_cost['instanceOf'])
            step.add_input(step0, name=soName.get(target))
            walk(node, visited, step0)

        for node in soMaps[target]:
            step0 = MappingStep(output_iri=target,
                                output_unit=soUnit.get(target))
            step0.steptype = StepType.MAPSTO
            step0.cost = soCost.get(target, default_cost['mapsTo'])
            step.add_input(step0, name=soName.get(target))
            walk(node, visited, step0)

        for node in osMaps[target]:
            step0 = MappingStep(output_iri=target,
                                output_unit=soUnit.get(target))
            step0.steptype = StepType.INV_MAPSTO
            step0.cost = soCost.get(target, default_cost['mapsTo'])
            step.add_input(step0, name=soName.get(target))
            walk(node, visited, step0)

        for node in osSubcl[target]:
            step0 = MappingStep(output_iri=target,
                                output_unit=soUnit.get(target))
            step0.steptype = StepType.INV_SUBCLASSOF
            step0.cost = soCost.get(target, default_cost['subClassOf'])
            step.add_input(step0, name=soName.get(target))
            walk(node, visited, step0)

        for fmap in function_mappers:
            for func, input_iris in fmap[target]:
                step0 = MappingStep(output_iri=target,
                                    output_unit=soUnit.get(target))
                step0.steptype = StepType.FUNCTION
                step0.cost = soCost.get(func, default_cost['function'])
                step.add_input(step0, name=soName.get(target))
                step0.join_mode = True
                for i, input_iri in enumerate(input_iris):
                    walk(input_iri, visited, step0)
                step0.join_inputs()

    visited = set()
    step = MappingStep(output_iri=target, output_unit=soUnit.get(target))
    if target in soInst:
        # It is only initially we want to follow instanceOf in forward direction.
        visited.add(target)  # do we really wan't this?
        source = soInst[target]
        step.steptype = StepType.INSTANCEOF
        step.cost = soCost.get(source, default_cost['instanceOf'])
        step0 = MappingStep(output_iri=source, output_unit=soUnit.get(source))
        step.add_input(step0, name=soName.get(target))
        target = source
    if target not in soMaps:
        raise MissingRelationError(f'Missing "mapsTo" relation on: {target}')
    step.steptype = StepType.MAPSTO
    step.cost = soCost.get(target, default_cost['mapsTo'])
    walk(target, visited, step)

    return step


    #step.output_unit = soUnit.get(target)
    #cls = getone(soMaps, target)
    #if target in soInst:
    #    target = soInst[target]
    #    step.cost = soCost.get(target, default_cost['instanceOf'])
    #    if step.output_unit is None:
    #        step.output_unit = soUnit.get(target)
    #    if not cls:
    #        cls = getone(soMaps, target)
    #if not cls:
    #    raise AmbiguousMappingError(
    #        f'no or more than one mappings defined on target: {target}')
    #step.cost = soCost.get(target, default_cost['mapsTo'])
    #step.steptype = 'mo:mapsTo'
    #visited.add(target)
    #walk(cls, visited, step)

    #def walk_forward(node, visited, laststep):
    #    """Walk forward from `entity` in the direction of mapsTo."""
    #    if node not in visited:
    #        walk_backward(node, visited, laststep)
    #        for e in sRo[entity]:
    #            walk_forward(
    #                e, visited.union(set([entity])),
    #                route + [(entity, mapsTo, e)])
    #        for e in oSCs[entity]:
    #            walk_forward(
    #                e, visited.union(set([entity])),
    #                route + [(e, subClassOf, entity)])
    #
    #def walk_backward(entity, visited, route):
    #    """Walk backward from `entity` to a source, against the direction of
    #    mapsTo."""
    #    if entity not in visited:
    #        if entity in sources:
    #            mapped_sources.append(entity)
    #            mapped_routes.append(route)
    #        else:
    #            for e in oRs[entity]:
    #                walk_backward(
    #                    e, visited.union(set([entity])),
    #                    route + [(e, mapsTo, entity)])
    #            for e in sSCo[entity]:
    #                walk_backward(
    #                    e, visited.union(set([entity])),
    #                    route + [(entity, subClassOf, e)])
    #
    #return step



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


unitconvert = unitconvert_pint



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
