"""Implements mappings between entities.

Units are currently handled with pint.Quantity.  The benefit of this
compared to explicit unit conversions, is that units will be handled
transparently by mapping functions, without any need to specify units
of input and output parameters.

Shapes are automatically handled by expressing non-scalar quantities
with numpy.

"""
from __future__ import annotations

import itertools
import types
import warnings
from collections import defaultdict
from collections.abc import Sequence
from enum import Enum
from typing import Any, Callable, Dict, List, Optional, Union

import numpy as np
from pint import Quantity
from tripper import Triplestore, DM, FNO, MAP, RDF, RDFS

import dlite
from dlite.utils import infer_dimensions


class MappingError(dlite.DLiteError):
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

    def show(self, routeno=None, name=None, indent=0):
        """Returns a string representation of the Value.

        Arguments:
            routeno: Unused.  The argument exists for consistency with
                the corresponding method in Step.
            name: Name of value.
            indent: Indentation level.
        """
        s = []
        ind = ' '*indent
        s.append(ind + f'{name if name else "Value"}:')
        s.append(ind + f'  iri: {self.iri}')
        s.append(ind + f'  property_iri: {self.property_iri}')
        s.append(ind + f'  unit: {self.unit}')
        s.append(ind + f'  cost: {self.cost}')
        s.append(ind + f'  value: {self.value}')
        return '\n'.join(s)


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
        output_unit: Output unit.

    The arguments can also be assigned as attributes.
    """
    def __init__(
            self,
            output_iri: Optional[str] = None,
            steptype: Optional[StepType] = None,
            function: Optional[Callable] = None,
            cost: Union[Any, Callable] = 1.0,
            output_unit: Optional[str] = None,
    ):
        self.output_iri = output_iri
        self.steptype = steptype
        self.function = function
        self.cost = cost
        self.output_unit = output_unit
        self.input_routes = []  # list of inputs dicts
        self.join_mode = False  # whether to join upcoming input
        self.joined_input = {}

    def add_inputs(self, inputs):
        """Add input dict for an input route."""
        assert isinstance(inputs, dict)
        self.input_routes.append(inputs)

    def add_input(self, input, name=None):
        """Add an input (MappingStep or Value), where `name` is the name
        assigned to the argument.

        If the `join_mode` attribute is false, a new route is created with
        only one input.

        If the `join_mode` attribute is true, the input is remembered, but
        first added when join_input() is called.
        """
        assert isinstance(input, (MappingStep, Value))
        argname = name if name else f'arg{len(self.joined_input)+1}'
        if self.join_mode:
            self.joined_input[argname] = input
        else:
            self.add_inputs({argname: input})

    def join_input(self):
        """Join all input added with add_input() since `join_mode` was set true.
        Resets `join_mode` to false."""
        if not self.join_mode:
            raise MappingError('Calling join_input() when join_mode is false.')
        self.join_mode = False
        self.add_inputs(self.joined_input)
        self.joined_input = {}

    def eval(self, routeno=None, unit=None, magnitude=False, quantity=Quantity):
        """Returns the evaluated value of given input route number.

        Args:
            routeno: The route number to evaluate.  If None (default)
                the route with the lowest cost is evalueated.
            unit: return the result in the given unit.
                Implies `magnitude=True`.
            magnitude: Whether to only return the magitude of the evaluated
                value (with no unit).
            quantity: Quantity class to use for evaluation.  Defaults to pint.
        """
        if routeno is None:
            (_, routeno), = self.lowest_costs(nresults=1)
        inputs, idx = self.get_inputs(routeno)
        values = get_values(inputs, idx, quantity=quantity)
        if self.function:
            value = self.function(**values)
        elif len(values) == 1:
            value, = values.values()
        else:
            raise TypeError(
                f"Expected inputs to be a single argument: {values}")

        if isinstance(value, quantity) and unit:
            return value.m_as(unit)
        elif isinstance(value, quantity) and magnitude:
            return value.m
        else:
            return value

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
        n = 0  # total number of routes

        # Loop over all toplevel routes leading into this mapping step
        for inputs in self.input_routes:

            # For each route, loop over all input arguments of this step
            # The number of permutations we must consider is the product
            # of the total number of routes to each input argument.
            #
            # We (potentially drastic) limit the possibilities by only
            # considering the `nresults` routes with lowest costs into
            # each argument.  This gives at maximum
            #
            #     nresults * number_of_input_arguments
            #
            # possibilities. We calculate the costs for all of them and
            # store them in an array with two columns: `cost` and `routeno`.
            # The `results` list is extended with the cost array
            # for each toplevel route leading into this step.
            base = np.rec.fromrecords([(0.0, 0)], names='cost,routeno',
                                      formats='f8,i8')
            m = 1
            for input in inputs.values():
                if isinstance(input, MappingStep):
                    nroutes = input.number_of_routes()
                    res = np.rec.fromrecords([row for row in sorted(
                        input.lowest_costs(nresults=nresults),
                        key=lambda x: x[1])], dtype=base.dtype)
                    res1 = res.repeat(len(base))
                    base = np.tile(base, len(res))
                    base.cost += res1.cost
                    base.routeno += res1.routeno * m
                    m *= nroutes
                else:
                    base.cost += input.cost

            # Reduce the length of base (makes probably only sense in
            # the case self.cost is a callable, but it doesn't hurt...)
            base.sort()
            base = base[:nresults]
            base.routeno += n
            n += m

            # Add the cost for this step to `res`.  If `self.cost` is
            # a callable, we call it with the input for each routeno
            # as arguments.  Otherwise `self.cost` is the cost of this
            # mapping step.
            if callable(self.cost):
                for i, rno in enumerate(base.routeno):
                    values = get_values(inputs, rno, magnitudes=True)
                    owncost = self.cost(**values)
                    base.cost[i] += owncost
            else:
                owncost = self.cost
                base.cost += owncost

            result.extend(base.tolist())

        # Finally sort the results according to cost and return the
        # `nresults` rows with lowest cost.
        return sorted(result)[:nresults]

    def show(self, routeno=None, name=None, indent=0):
        """Returns a string representation of the mapping routes to this step.

        Arguments:
            routeno: show given route.  The default is to show all routes.
            name: Name of the last mapping step (mainly for internal use).
            indent: How of blanks to prepend each line with (mainly for
                internal use).
        """
        s = []
        ind = ' '*indent
        s.append(ind + f'{name if name else "Step"}:')
        s.append(ind + f'  steptype: '
                 f'{self.steptype.name if self.steptype else None}')
        s.append(ind + f'  output_iri: {self.output_iri}')
        s.append(ind + f'  output_unit: {self.output_unit}')
        s.append(ind + f'  cost: {self.cost}')
        if routeno is None:
            s.append(ind + f'  routes:')
            for inputs in self.input_routes:
                t = '\n'.join([input_.show(name=name_, indent=indent+6)
                               for name_, input_ in inputs.items()])
                s.append(ind + '    - ' + t[indent+6:])
        else:
            s.append(ind + f'  inputs:')
            inputs, idx = self.get_inputs(routeno)
            t = '\n'.join([input_.show(routeno=idx, name=name_, indent=indent+6)
                           for name_, input_ in inputs.items()])
            s.append(ind + '    - ' + t[indent+6:])
        return '\n'.join(s)


def get_nroutes(inputs):
    """Help function returning the number of routes for an input dict."""
    m = 1
    for input in inputs.values():
        if isinstance(input, MappingStep):
            m *= input.number_of_routes()
    return m


def get_values(inputs, routeno, quantity=Quantity, magnitudes=False):
    """Help function returning a dict mapping the input names to actual value
    of expected input unit.

    There exists `get_nroutes(inputs)` routes to populate `inputs`.
    `routeno` is the index of the specific route we will use to obtain the
    values."""
    values = {}
    for k, v in inputs.items():
        if isinstance(v, MappingStep):
            value = v.eval(routeno=routeno, quantity=quantity)
            values[k] = (
                value.to(v.output_unit)
                if v.output_unit and isinstance(v, quantity) else value
            )
        else:
            values[k] = quantity(v.value, v.unit)

        if magnitudes:
            values = {k: v.m if isinstance(v, quantity) else v
                      for k, v in values.items()}

    return values


def fno_mapper(triplestore):
    """Finds all function definitions in `triplestore` based on the function
    ontololy (FNO).

    Return a dict mapping output IRIs to a list of

        (function_iri, [input_iris, ...])

    tuples.
    """
    # Temporary dicts for fast lookup
    Dfirst = {s: o for s, o in triplestore.subject_objects(RDF.first)}
    Drest = {s: o for s, o in triplestore.subject_objects(RDF.rest)}
    Dexpects = defaultdict(list)
    Dreturns = defaultdict(list)
    for s, o in triplestore.subject_objects(FNO.expects):
        Dexpects[s].append(o)
    for s, o in triplestore.subject_objects(FNO.returns):
        Dreturns[s].append(o)

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
        target,
        sources,
        triplestore,
        function_repo=None,
        function_mappers=(fno_mapper, ),
        default_costs={'function': 10.0, 'mapsTo': 2.0, 'instanceOf': 1.0,
                       'subClassOf': 1.0, 'value': 0.0},
        mapsTo=MAP.mapsTo,
        instanceOf=DM.instanceOf,
        subClassOf=RDFS.subClassOf,
        #description=DCTERMS.description,
        label=RDFS.label,
        hasUnit=DM.hasUnit,
        hasCost=DM.hasCost,  # TODO - add hasCost to the DM ontology
):
    """Find routes of mappings from any source in `sources` to `target`.

    This implementation supports functions (using FnO) and subclass
    relations.  It also correctly handles transitivity of `mapsTo` and
    `subClassOf` relations.

    Arguments:
        target: IRI of the target in `triplestore`.
        sources: Dict mapping source IRIs to source values.
        triplestore: Triplestore instance.
            It is safe to pass a generator expression too.

    Additional arguments for fine-grained tuning:
        function_repo: Dict mapping function IRIs to corresponding Python
            function.  Default is to use `triplestore.function_repo`.
        function_mappers: Sequence of mapping functions that takes
            `triplestore` as argument and return a dict mapping output IRIs
            to a list of `(function_iri, [input_iris, ...])` tuples.
        default_costs: A dict providing default costs of different types
            of mapping steps ("function", "mapsTo", "instanceOf",
            "subclassOf", and "value").  These costs can be overridden with
            'hasCost' relations in the ontology.
        mapsTo: IRI of 'mapsTo' in `triplestore`.
        instanceOf: IRI of 'instanceOf' in `triplestore`.
        subClassOf: IRI of 'subClassOf' in `triples`.  Set it to None if
            subclasses should not be considered.
        label: IRI of 'label' in `triplestore`.  Used for naming function
            input parameters.  The default is to use rdfs:label.
        hasUnit: IRI of 'hasUnit' in `triples`.  Can be used to explicit
            specify the unit of a quantity.
        hasCost: IRI of 'hasCost' in `triples`.  Used for associating a
            user-defined cost or cost function with instantiation of a
            property.

    Returns:
        A MappingStep instance.  This is a root of a nested tree of
        MappingStep instances providing an (efficient) internal description
        of all possible mapping routes from `sources` to `target`.
    """
    if function_repo is None:
        function_repo = triplestore.function_repo

    # Create lookup tables for fast access to properties
    # This only transverse `tiples` once
    soMaps  = defaultdict(list)  # (s, mapsTo, o)     ==> soMaps[s]  -> [o, ..]
    osMaps  = defaultdict(list)  # (o, mapsTo, s)     ==> osMaps[o]  -> [s, ..]
    osSubcl = defaultdict(list)  # (o, subClassOf, s) ==> osSubcl[o] -> [s, ..]
    soInst  = dict()             # (s, instanceOf, o) ==> soInst[s]  -> o
    osInst  = defaultdict(list)  # (o, instanceOf, s) ==> osInst[o]  -> [s, ..]
    for s, o in triplestore.subject_objects(mapsTo):
        soMaps[s].append(o)
        osMaps[o].append(s)
    for s, o in triplestore.subject_objects(subClassOf):
        osSubcl[o].append(s)
    for s, o in triplestore.subject_objects(instanceOf):
        if s in soInst:
            raise InconsistentTriplesError(
                f'The same individual can only relate to one datamodel '
                f'property via {instanceOf} relations.')
        soInst[s] = o
        osInst[o].append(s)
    soName = {s: o for s, o in triplestore.subject_objects(label)}
    soUnit = {s: o for s, o in triplestore.subject_objects(hasUnit)}
    soCost = {s: o for s, o in triplestore.subject_objects(hasCost)}

    def getcost(target, stepname):
        """Returns the cost assigned to IRI `target` for a mapping step
        of type `stepname`."""
        cost = soCost.get(target, default_costs[stepname])
        if cost is None:
            return None
        return function_repo[cost] if cost in function_repo else float(cost)

    def walk(target, visited, step):
        """Walk backward in rdf graph from `node` to sources."""
        if target in visited: return
        visited.add(target)

        def addnode(node, steptype, stepname):
            if node in visited:
                return
            step.steptype = steptype
            step.cost = getcost(target, stepname)
            if node in sources:
                value = Value(value=sources[node], unit=soUnit.get(node),
                              iri=node, property_iri=soInst.get(node),
                              cost=getcost(node, 'value'))
                step.add_input(value, name=soName.get(node))
            else:
                prevstep = MappingStep(output_iri=node,
                                       output_unit=soUnit.get(node))
                step.add_input(prevstep, name=soName.get(node))
                walk(node, visited, prevstep)

        for node in osInst[target]:
            addnode(node, StepType.INV_INSTANCEOF, 'instanceOf')

        for node in soMaps[target]:
            addnode(node, StepType.MAPSTO, 'mapsTo')

        for node in osMaps[target]:
            addnode(node, StepType.INV_MAPSTO, "mapsTo")

        for node in osSubcl[target]:
            addnode(node, StepType.INV_SUBCLASSOF, 'subClassOf')

        for fmap in function_mappers:
            for func, input_iris in fmap(triplestore)[target]:
                step.steptype = StepType.FUNCTION
                step.cost = getcost(func, 'function')
                step.function = function_repo[func]
                step.join_mode = True
                for i, input_iri in enumerate(input_iris):
                    step0 = MappingStep(output_iri=input_iri,
                                        output_unit=soUnit.get(input_iri))
                    step.add_input(step0, name=soName.get(input_iri))
                    walk(input_iri, visited, step0)
                step.join_input()

    visited = set()
    step = MappingStep(output_iri=target, output_unit=soUnit.get(target))
    if target in soInst:
        # It is only initially we want to follow instanceOf in forward
        # direction.
        visited.add(target)  # do we really wan't this?
        source = soInst[target]
        step.steptype = StepType.INSTANCEOF
        step.cost = getcost(source, 'instanceOf')
        step0 = MappingStep(output_iri=source, output_unit=soUnit.get(source))
        step.add_input(step0, name=soName.get(target))
        step = step0
        target = source
    if target not in soMaps:
        raise MissingRelationError(f'Missing "mapsTo" relation on: {target}')
    walk(target, visited, step)

    return step


def instance_routes(meta, instances, triplestore, allow_incomplete=False,
                    quantity=Quantity, **kwargs):
    """Find all mapping routes for populating an instance of `meta`.

    Arguments:
        meta: Metadata for the instance we will create.
        instances: sequence of instances that the new intance will be
            populated from.
        triplestore: Triplestore containing the mappings.
        allow_incomplete: Whether to allow not populating all properties
            of the returned instance.
        quantity: Class implementing quantities with units.  Defaults to
            pint.Quantity.
        kwargs: Keyword arguments passed to mapping_route().

    Returns:
        A dict mapping property names to a MappingStep instance.
    """
    if isinstance(meta, str):
        meta = dlite.get_instance(meta)
    if isinstance(instances, dlite.Instance):
        instances = [instances]

    sources = {}
    for inst in instances:
        props = {p.name: p for p in inst.meta['properties']}
        for k, v in inst.properties.items():
            sources[f'{inst.meta.uri}#{k}'] = quantity(v, props[k].unit)

    routes = {}
    for prop in meta['properties']:
        target = f'{meta.uri}#{prop.name}'
        try:
            route = mapping_route(target, sources, triplestore, **kwargs)
        except MissingRelationError:
            if allow_incomplete:
                continue
            raise
        if not allow_incomplete and not route.number_of_routes():
            raise InsufficientMappingError(f'no mappings for {target}')
        routes[prop.name] = route

    return routes


def instantiate_from_routes(meta, routes, routedict=None, id=None,
                            quantity=Quantity):
    """Create a new instance of `meta` from selected mapping route returned
    by instance_routes().

    Arguments:
        meta: Metadata to instantiate.
        routes: Dict returned by instance_routes().  It should map property
            names to MappingStep instances.
        routedict: Dict mapping property names to route number to select for
            the given property.  The default is to select the route with
            lowest cost.
        id: URI of instance to create.
        quantity: Class implementing quantities with units.  Defaults to
            pint.Quantity.

    Returns:
        New instance.
    """
    if isinstance(meta, str):
        meta = dlite.get_instance(meta)

    if routedict is None:
        routedict = {}

    values = {}
    for prop in meta['properties']:
        if prop.name in routes:
            step = routes[prop.name]
            values[prop.name] = step.eval(routeno=routedict.get(prop.name),
                                          unit=prop.unit,
                                          quantity=quantity)
    dims = infer_dimensions(meta, values)
    inst = meta(dims=dims, id=id)

    for k, v in routes.items():
        inst[k] = v.eval(magnitude=True, unit=meta.getprop(k).unit)

    return inst


def instantiate(meta, instances, triplestore, routedict=None, id=None,
                allow_incomplete=False, quantity=Quantity, **kwargs):
    """Create a new instance of `meta` populated with the selected mapping
    routes.

    This is a convenient function that combines instance_routes() and
    instantiate_from_routes().  If you want to investigate the possible
    routes, you will probably want to call instance_routes() and
    instantiate_from_routes() instead.

    Arguments:
        meta: Metadata to instantiate.
        instances: Sequence of instances with source values.
        triplestore: Triplestore instance.
            It is safe to pass a generator expression too.
        routedict: Dict mapping property names to route number to select for
            the given property.  The default is to select the route with
            lowest cost.
        id: URI of instance to create.
        allow_incomplete: Whether to allow not populating all properties
            of the returned instance.
        quantity: Class implementing quantities with units.  Defaults to
            pint.Quantity.

    Keyword arguments (passed to mapping_route()):
        function_repo: Dict mapping function IRIs to corresponding Python
            function.  Default is to use `triplestore.function_repo`.
        function_mappers: Sequence of mapping functions that takes
            `triplestore` as argument and return a dict mapping output IRIs
            to a list of `(function_iri, [input_iris, ...])` tuples.
        mapsTo: IRI of 'mapsTo' in `triplestore`.
        instanceOf: IRI of 'instanceOf' in `triplestore`.
        subClassOf: IRI of 'subClassOf' in `triplestore`.  Set it to None if
            subclasses should not be considered.
        label: IRI of 'label' in `triplestore`.  Used for naming function
            input parameters.  The default is to use rdfs:label.
        hasUnit: IRI of 'hasUnit' in `triplestore`.
        hasCost: IRI of 'hasCost' in `triplestore`.

    Returns:
        New instance.
    """
    meta = dlite.get_instance(meta)

    routes = instance_routes(
        meta=meta,
        instances=instances,
        triplestore=triplestore,
        allow_incomplete=allow_incomplete,
        quantity=quantity,
        **kwargs
    )
    return instantiate_from_routes(
        meta=meta,
        routes=routes,
        routedict=routedict,
        id=id,
        quantity=quantity
    )


def instantiate_all(meta, instances, triplestore, routedict=None,
                allow_incomplete=False, quantity=Quantity, **kwargs):
    """Like instantiate(), but returns a generator over all possible instances.

    The number of instances iterated over is the product of the number
    routes for each property and may potentially be very large.  Use
    `routedict` to reduct the number.

    Arguments:
        meta: Metadata to instantiate.
        instances: Sequence of instances with source values.
        triplestore: Triplestore instance.
            It is safe to pass a generator expression too.
        routedict: Dict mapping property names to route number to select for
            the given property.  For property names not in `routedict`, the
            default is to iterate over all possible routes.  Set the value to
            None to only consider the route with lowest cost.
        allow_incomplete: Whether to allow not populating all properties
            of the returned instance.
        quantity: Class implementing quantities with units.  Defaults to
            pint.Quantity.

    Keyword arguments (passed to mapping_route()):
        function_repo: Dict mapping function IRIs to corresponding Python
            function.  Default is to use `triplestore.function_repo`.
        function_mappers: Sequence of mapping functions that takes
            `triplestore` as argument and return a dict mapping output IRIs
            to a list of `(function_iri, [input_iris, ...])` tuples.
        mapsTo: IRI of 'mapsTo' in `triplestore`.
        instanceOf: IRI of 'instanceOf' in `triplestore`.
        subClassOf: IRI of 'subClassOf' in `triplestore`.  Set it to None if
            subclasses should not be considered.
        label: IRI of 'label' in `triplestore`.  Used for naming function
            input parameters.  The default is to use rdfs:label.
        hasUnit: IRI of 'hasUnit' in `triplestore`.
        hasCost: IRI of 'hasCost' in `triplestore`.

    Returns:
        Generator over new instances.
    """
    meta = dlite.get_instance(meta)

    routes = instance_routes(
        meta=meta,
        instances=instances,
        triplestore=triplestore,
        allow_incomplete=allow_incomplete,
        quantity=quantity,
        **kwargs
    )

    property_names = [p.name for p in meta.properties['properties']]
    nprops = len(property_names)

    def routedicts(n):
        """Recursive help function returning an iterator over all possible
        routedicts.  `n` is the index of the current property."""
        if n < 0:
            yield {}
            return
        for outer in routedicts(n-1):
            name = property_names[n]
            if routedict and name in routedict:
                outer[name] = routedict[name]
                yield outer
            else:
                step = routes[name]
                for inner in range(step.number_of_routes()):
                    outer[name] = inner
                    yield outer

    for rd in routedicts(len(property_names) - 1):
        yield instantiate_from_routes(
            meta=meta,
            routes=routes,
            routedict=rd,
            quantity=quantity
        )




# ------------- Old implementation -----------------

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
    warnings.warn(
        "make_instance() is deprecated. Use instantiate() instead.",
        DeprecationWarning,
    )

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
