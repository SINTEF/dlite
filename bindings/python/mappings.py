"""Implements mappings between entities.

Units are currently handled with pint.Quantity.  The benefit of this
compared to explicit unit conversions, is that units will be handled
transparently by mapping functions, without any need to specify units
of input and output parameters.

Shapes are automatically handled by expressing non-scalar quantities
with numpy.

"""
from __future__ import annotations

import warnings
from collections import defaultdict
from enum import Enum
from typing import TYPE_CHECKING

import numpy as np
from pint import Quantity
from tripper import DM, FNO, MAP, RDF, RDFS, Namespace

import dlite
from dlite.utils import infer_dimensions

from tripper.mappings import Value, MappingStep, StepType, mapping_routes
from tripper.mappings.mappings import (
    MappingError,
    InsufficientMappingError,
    AmbiguousMappingError,
    InconsistentDimensionError,
    InconsistentTriplesError,
    MissingRelationError,
    UnknownUnitError,
)

if TYPE_CHECKING:  # pragma: no cover
    from typing import Any, Callable, Generator, Optional, Sequence, Type

    from tripper.triplestore import Triplestore


def instance_routes(
    meta: str | dlite.Metadata | Namespace,
    instances: "dlite.Instance | Sequence[dlite.Instance]",
    triplestore: "Triplestore",
    allow_incomplete: bool = False,
    quantity: "Type[Quantity]" = Quantity,
    **kwargs,
) -> dict[str, MappingStep]:
    """Find all mapping routes for populating an instance of `meta`.

    Arguments:
        meta: Metadata for the instance we will create.
        instances: sequence of instances that the new instance will be
            populated from.
        triplestore: Triplestore containing the mappings.
        allow_incomplete: Whether to allow not populating all properties
            of the returned instance.
        quantity: Class implementing quantities with units.  Defaults to
            pint.Quantity.
        kwargs: Keyword arguments passed to mapping_routes().

    Returns:
        A dict mapping property names to a MappingStep instance.

    """
    if isinstance(meta, str):
        meta = dlite.get_instance(meta)
    elif isinstance(meta, Namespace):
        meta = dlite.get_instance(str(meta).rstrip("/#"))

    if isinstance(instances, dlite.Instance):
        instances = [instances]

    sources = {}
    for inst in instances:
        props = {prop.name: prop for prop in inst.meta["properties"]}
        for key, value in inst.properties.items():
            if props[key].unit:

                try:
                    q = quantity(1.0, props[key].unit)
                except TypeError:
                    raise UnknownUnitError(
                        f"unknown unit '{props[key].unit}' in datamodel: "
                        f"{inst.meta.uri}"
                    )

                sources[f"{inst.meta.uri}#{key}"] = quantity(
                    value, props[key].unit
                )

            else:
                sources[f"{inst.meta.uri}#{key}"] = value

    routes = {}
    for prop in meta["properties"]:
        target = f"{meta.uri}#{prop.name}"
        try:
            route = mapping_routes(target, sources, triplestore, **kwargs)
        except MissingRelationError:
            if allow_incomplete:
                continue
            raise
        if not allow_incomplete and not route.number_of_routes():
            raise InsufficientMappingError(f"No mappings for {target}")
        routes[prop.name] = route

    return routes


def instantiate_from_routes(
    meta: str | dlite.Metadata | Namespace,
    routes: dict[str, MappingStep],
    routedict: dict[str, int] = None,
    id: "Optional[str]" = None,
    quantity: "Type[Quantity]" = Quantity,
) -> dlite.Instance:
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
    elif isinstance(meta, Namespace):
        meta = dlite.get_instance(str(meta).rstrip("/#"))

    routedict = routedict or {}

    values = {}
    for prop in meta["properties"]:
        if prop.name in routes:
            step = routes[prop.name]
            values[prop.name] = step.eval(
                routeno=routedict.get(prop.name),
                unit=prop.unit,
                quantity=quantity,
            )
    shape = infer_dimensions(meta, values)
    inst = meta(shape=shape, id=id)

    for key, value in routes.items():
        inst[key] = value.eval(magnitude=True, unit=meta.getprop(key).unit)

    return inst


def instantiate(
    meta: str | dlite.Metadata | Namespace,
    instances: "dlite.Instance | Sequence[dlite.Instance]",
    triplestore: "Triplestore",
    routedict: "Optional[dict[str, int]]" = None,
    id: "Optional[str]" = None,
    allow_incomplete: bool = False,
    quantity: "Type[Quantity]" = Quantity,
    **kwargs,
) -> dlite.Instance:
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

    Returns:
        New instance.

    """
    routes = instance_routes(
        meta=meta,
        instances=instances,
        triplestore=triplestore,
        allow_incomplete=allow_incomplete,
        quantity=quantity,
        **kwargs,
    )
    return instantiate_from_routes(
        meta=meta,
        routes=routes,
        routedict=routedict,
        id=id,
        quantity=quantity,
    )


def instantiate_all(
    meta: str | dlite.Metadata | Namespace,
    instances: "dlite.Instance | Sequence[dlite.Instance]",
    triplestore: "Triplestore",
    routedict: "Optional[dict[str, int]]" = None,
    allow_incomplete: bool = False,
    quantity: "Type[Quantity]" = Quantity,
    **kwargs,
) -> "Generator[dlite.Instance, None, None]":
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

    Yields:
        New instances.

    """
    if isinstance(meta, str):
        meta = dlite.get_instance(meta)
    elif isinstance(meta, Namespace):
        meta = dlite.get_instance(str(meta).rstrip("/#"))

    routes = instance_routes(
        meta=meta,
        instances=instances,
        triplestore=triplestore,
        allow_incomplete=allow_incomplete,
        quantity=quantity,
        **kwargs,
    )

    property_names = [prop.name for prop in meta.properties["properties"]]

    def routedicts(n: int) -> "Generator[dict[str, int], None, None]":
        """Recursive help function returning an iterator over all possible
        routedicts.  `n` is the index of the current property."""
        if n < 0:
            yield {}
            return
        for outer in routedicts(n - 1):
            name = property_names[n]
            if routedict and name in routedict:
                outer[name] = routedict[name]
                yield outer
            else:
                step = routes[name]
                for inner in range(step.number_of_routes()):
                    outer[name] = inner
                    yield outer

    for route_dict in routedicts(len(property_names) - 1):
        yield instantiate_from_routes(
            meta=meta, routes=routes, routedict=route_dict, quantity=quantity
        )


# ------------- Old implementation -----------------

def unitconvert_pint(dest_unit: "Any", value: "Any", unit: "Any") -> "Any":
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


def match_factory(
    triples: "Sequence[tuple[str, str, str]]", match_first: bool = False
) -> "Callable[[Optional[str], Optional[str], Optional[str]], Generator[tuple[str, str, str], None, None]]":
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

    def match(
        s: "Optional[str]" = None, p: "Optional[str]" = None, o: "Optional[str]" = None
    ) -> "Generator[tuple[str, str, str], None, None]":
        """Returns generator over all triples that matches (s, p, o)."""
        return (
            triple for triple in triples
            if (
                (s is None or triple[0] == s) and
                (p is None or triple[1] == p) and
                (o is None or triple[2] == o)
            )
        )

    if match_first:
        return lambda s=None, p=None, o=None: next(
            iter(match(s, p, o) or ()), (None, None, None))

    return match


def assign_dimensions(
    shape: dict[str, int],
    inst: dlite.Instance,
    propname: str,
) -> None:
    """Assign dimensions from property assignment.

    Args:
        shape: A dict with (dimension name, dimension value) pairs that
          should be assigned.  Only values that are None will be assigned.
        inst: Source instance.
        propname: Source property name.

    """
    lst = [prop.shape for prop in inst.meta['properties'] if prop.name == propname]
    if not lst:
        raise MappingError(f"Unexpected property name: {propname}")

    src_dims, = lst
    for dim in src_dims:
        if dim not in shape:
            raise InconsistentDimensionError(f"Unexpected dimension: {dim}")

        if shape[dim] is None:
            shape[dim] = inst.dimensions[dim]
        elif shape[dim] != inst.dimensions[dim]:
            raise InconsistentDimensionError(
                f"Trying to assign dimension {dim} of {inst.meta.uri} "
                f"to {inst.dimensions[dim]}, but it is already assigned "
                f"to {shape[dim]}"
            )


def make_instance(
    meta: dlite.Metadata,
    instances: "dlite.Instance | Sequence[dlite.Instance]",
    mappings: "Sequence[tuple[str, str, str]]" = (),
    strict: bool = True,
    allow_incomplete: bool = False,
    mapsTo: str = ':mapsTo',
) -> dlite.Instance:
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
      triples. Do we really want that?  May be useful, but will add
      a lot of complexity. Should we have different relations for
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

    shape = {dim.name: None for dim in meta['dimensions']}
    props = {}

    for prop in meta['properties']:
        prop_uri = f'{meta.uri}#{prop.name}'
        for _, _, o in match(prop_uri, mapsTo, None):
            for inst in instances:
                for prop2 in inst.meta['properties']:
                    prop2_uri = f'{inst.meta.uri}#{prop2.name}'
                    for _ in match(prop2_uri, mapsTo, o):
                        value = inst[prop2.name]
                        if prop.name not in props:
                            assign_dimensions(shape, inst, prop2.name)
                            props[prop.name] = value
                        elif props[prop.name] != value:
                            raise AmbiguousMappingError(
                                f"{prop.name!r} maps to both "
                                f"{props[prop.name]!r} and {value!r}"
                            )

        if prop.name not in props and not strict:
            for inst in instances:
                if prop.name in inst.properties:
                    value = inst[prop.name]
                    if prop.name not in props:
                        assign_dimensions(shape, inst, prop.name)
                        props[prop.name] = value
                    elif props[prop.name] != value:
                        raise AmbiguousMappingError(
                            f"{prop.name!r} assigned to both "
                            f"{props[prop.name]!r} and {value!r}"
                        )

        if not allow_incomplete and prop.name not in props:
            raise InsufficientMappingError(
                f"no mapping for assigning property {prop.name!r} "
                f"in {meta.uri}"
            )

    if None in shape:
        dimname = [name for name, dim in shape.items() if dim is None][0]
        raise InsufficientMappingError(
            f"dimension {dimname!r} is not assigned"
        )

    inst = meta(list(shape.values()))
    for key, value in props.items():
        inst[key] = value
    return inst
