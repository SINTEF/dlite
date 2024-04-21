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
    routedict: "Optional[dict[str, int]]" = None,
    id: "Optional[str]" = None,
    default: "Optional[dlite.Instance]" = None,
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
        default: A dlite instance with default values for unassigned
            properties.
        quantity: Class implementing quantities with units.  Defaults to
            pint.Quantity.

    Returns:
        New instance.

    """
    if isinstance(meta, str):
        meta = dlite.get_instance(meta)
    elif isinstance(meta, Namespace):
        meta = dlite.get_instance(str(meta).rstrip("/#"))

    if default and default.meta.uri != meta.uri:
        raise f"`default` must be an instance of {meta.uri}"

    if routedict is None:
        routedict = {}

    values = {}
    for prop in meta["properties"]:
        if prop.name in routes:
            step = routes[prop.name]
            try:
                value = step.eval(
                    routeno=routedict.get(prop.name),
                    unit=prop.unit,
                    quantity=quantity,
                )
            except MissingRelationError:
                if not default:
                    raise
                value = default[prop.name]
        elif default:
            value = default[prop.name]

        values[prop.name] = value

    dimensions = infer_dimensions(meta, values)
    inst = meta(dimensions=dimensions, id=id)

    for key, value in values.items():
        inst[key] = value

    return inst


def instantiate(
    meta: str | dlite.Metadata | Namespace,
    instances: "dlite.Instance | Sequence[dlite.Instance]",
    triplestore: "Triplestore",
    routedict: "Optional[dict[str, int]]" = None,
    id: "Optional[str]" = None,
    allow_incomplete: bool = False,
    default: "Optional[dlite.Instance]" = None,
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
        default: A dlite instance with default values for unassigned
            properties.  Implies `allow_incomplete=True`.
        quantity: Class implementing quantities with units.  Defaults to
            pint.Quantity.

    Returns:
        New instance.

    """
    if default:
        allow_incomplete = True

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
        default=default,
        quantity=quantity,
    )


def instantiate_all(
    meta: str | dlite.Metadata | Namespace,
    instances: "dlite.Instance | Sequence[dlite.Instance]",
    triplestore: "Triplestore",
    routedict: "Optional[dict[str, int]]" = None,
    allow_incomplete: bool = False,
    default: "Optional[dlite.Instance]" = None,
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
        default: A dlite instance with default values for unassigned
            properties.  Implies `allow_incomplete=True`.
        quantity: Class implementing quantities with units.  Defaults to
            pint.Quantity.

    Yields:
        New instances.

    """
    if isinstance(meta, str):
        meta = dlite.get_instance(meta)
    elif isinstance(meta, Namespace):
        meta = dlite.get_instance(str(meta).rstrip("/#"))

    if default:
        allow_incomplete = True

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
            elif name in routes:
                step = routes[name]
                nroutes = step.number_of_routes()
                if nroutes:
                    for inner in range(nroutes):
                        outer[name] = inner
                        yield outer
                elif allow_incomplete:
                    yield outer
            elif allow_incomplete:
                yield outer

    for route_dict in routedicts(len(property_names) - 1):
        yield instantiate_from_routes(
            meta=meta,
            routes=routes,
            routedict=route_dict,
            default=default,
            quantity=quantity,
        )
