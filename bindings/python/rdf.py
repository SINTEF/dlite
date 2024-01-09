"""Module for representing DLite entities and instances with rdflib.

It uses the datamodel ontology: https://github.com/emmo-repo/datamodel-ontology
"""
import itertools
import pathlib
import re

import rdflib
from rdflib import Literal, RDF, URIRef
from rdflib.util import guess_format

import dlite


# Logical namespace to use as document base when parsing
PUBLIC_ID = "http://onto-ns.com/data/"

# Namespaces
DM = rdflib.Namespace("http://emmo.info/datamodel/0.0.2#")


def _is_valid_uri(value):
    """Returns true if `value` is a valid URI."""
    return bool(
        re.match(r"^[a-zA-Z+-]+://[a-zA-Z][a-zA-Z0-9_.+/#?,;=-]*$", value)
    )


def _get_uri(inst, base_uri):
    """Returns a uri for `inst`."""
    if inst.uri:
        if _is_valid_uri(inst.uri) or not base_uri:
            return inst.uri
        else:
            return base_uri + inst.uri
    elif base_uri:
        return base_uri + inst.uuid
    else:
        return inst.uuid


def _value(graph, subject=None, predicate=None, object=None, **kwargs):
    """Wrapper around rdflib.Graph.value() that raises an exception
    if the value is missing."""
    value = graph.value(
        subject=subject, predicate=predicate, object=object, **kwargs
    )
    if not value:
        raise ValueError(
            f"missing value for subject={subject}, predicate={predicate}, "
            f"object={object}"
        )
    return value


def _ref(value):
    """Return a URIRef if value is a valid URI, else a literal."""
    return URIRef(value) if _is_valid_uri(value) else Literal(value)


def to_graph(
    inst, graph=None, base_uri="", base_prefix=None, include_meta=None
):
    """Serialise DLite instance to a rdflib Graph object.

    Arguments:
        inst: Instance to serialise.
        graph: A rdflib Graph object that rdf triples for `inst` are added to.
            If None, a new Graph object is created.
        base_uri: Base URI that is prepended to the instance UUID or URI
            (if it is not already a valid URI).
        base_prefix: Optional namespace prefix to use for `base_uri`.
        include_meta: Whether to also serialise metadata.  The default
            is to only include metadata if `inst` is a data object.

    Returns:
        A rdflib Graph object.
    """
    if graph is None:
        graph = rdflib.Graph()
        graph.bind("dm", DM)
    if include_meta or (include_meta is None and inst.is_data):
        to_graph(inst.meta, graph)
    if base_uri and base_uri[-1] not in "#/":
        base_uri += "/"
    if base_prefix is not None and base_uri not in [
        str(v) for _, v in graph.namespaces()
    ]:
        graph.bind(base_prefix, base_uri)

    this = URIRef(_get_uri(inst, base_uri))
    sep = "/" if "#" in this else "#"

    graph.add((this, DM.hasUUID, Literal(inst.uuid)))
    graph.add((this, DM.instanceOf, URIRef(inst.meta.uri)))
    if inst.is_data:
        graph.add((this, RDF.type, DM.DataInstance))
        for k, v in inst.dimensions.items():
            dim = URIRef(this + sep + k)
            graph.add((this, DM.hasDimension, dim))
            # Add type (DimensionInstance)?
            graph.add((dim, DM.instanceOf, URIRef(f"{inst.meta.uri}#{k}")))
            graph.add((dim, DM.hasLabel, Literal(k)))
            graph.add((dim, DM.hasValue, Literal(v)))
        for k in inst.properties:
            prop = URIRef(this + sep + k)
            graph.add((this, DM.hasProperty, prop))
            # Add type (PropertyInstance)?
            graph.add((prop, DM.instanceOf, URIRef(f"{inst.meta.uri}#{k}")))
            graph.add((prop, DM.hasLabel, Literal(k)))
            graph.add(
                (
                    prop,
                    DM.hasValue,
                    Literal(inst.get_property_as_string(k, flags=1)),
                )
            )
    else:
        graph.add(
            (
                this,
                RDF.type,
                DM.Entity
                if inst.meta.uri == dlite.ENTITY_SCHEMA
                else DM.Metadata,
            )
        )
        if inst.description:
            graph.add(
                (this, DM.hasDescription, Literal(inst.description, lang="en"))
            )
        for d in inst.properties["dimensions"]:
            dim = URIRef(this + sep + d.name)
            graph.add((this, DM.hasDimension, dim))
            graph.add((dim, RDF.type, DM.Dimension))
            graph.add(
                (dim, DM.hasDescription, Literal(d.description, lang="en"))
            )
            graph.add((dim, DM.hasLabel, Literal(d.name, lang="en")))
        for p in inst.properties["properties"]:
            prop = URIRef(this + sep + p.name)
            graph.add((this, DM.hasProperty, prop))
            graph.add((prop, RDF.type, DM.Property))
            graph.add((prop, DM.hasLabel, Literal(p.name)))
            graph.add((prop, DM.hasType, _ref(p.type)))  # FIXME
            if p.ndims:
                shape = URIRef(f"{prop}/shape")
                graph.add((prop, DM.hasShape, shape))
                graph.add((shape, RDF.type, DM.Shape))
                prev, rel = shape, DM.hasFirst
                for i, expr in enumerate(p.shape):
                    dimexpr = URIRef(f"{shape}_{i}")
                    graph.add((prev, rel, dimexpr))
                    graph.add((dimexpr, RDF.type, DM.DimensionExpression))
                    graph.add((dimexpr, DM.hasValue, Literal(expr)))
                    prev, rel = dimexpr, DM.hasNext
            if p.unit:
                graph.add((prop, DM.hasUnit, _ref(p.unit)))  # FIXME
            if p.description:
                graph.add((prop, DM.hasDescription, Literal(p.description)))

    return graph


def to_rdf(
    inst,
    destination=None,
    format="turtle",
    base_uri="",
    base_prefix=None,
    include_meta=None,
    decode=True,
    **kwargs,
):
    """Serialise DLite instance to string.

    Arguments:
        inst: Instance to serialise.
        destination: File name or file object to serialise `inst` to.
            If None, the serialisation of `inst` is returned.
        format: Format to serialise to.  Build formats: "xml", "n3", "turtle",
           "nt", "pretty-xml", "trix", "trig", "nquads".
        base_uri: Base URI that is prepended to the instance UUID or URI
            (if it is not already a valid URI).
        base_prefix: Optional namespace prefix to use for `base_uri`.
        include_meta: Whether to also serialise metadata.  The default
            is to only include metadata if `inst` is a data object.
        decode: Whether to decode the resulting bytes object into a string.
            For rdflib 6.0 or newer, use the `encoding` option instead.
        kwargs: Additional arguments to pass to rdflib.Graph.serialize().

    Returns:
        The serialised instance if `destination is not None.
        If `decode` is true, a string is returned, otherwise a bytes object.
    """
    graph = to_graph(
        inst,
        base_uri=base_uri,
        base_prefix=base_prefix,
        include_meta=include_meta,
    )
    if isinstance(destination, pathlib.PurePath):
        destination = str(destination)
    s = graph.serialize(destination=destination, format=format, **kwargs)
    if isinstance(s, bytes) and decode:
        return s.decode()
    return s


def from_graph(graph, id=None):
    """Instantiate DLite instance with given id from rdflib Graph object.

    If `id` is None and `graph` only contain one instance, that instance
    is returned.  It is an error if `id` is None and `graph` contains more
    than one instances.

    Returns new DLite instance.
    """
    if id is None:
        g = itertools.chain(
            graph.subjects(RDF.type, DM.Entity),
            graph.subjects(RDF.type, DM.DataInstance),
            graph.subjects(RDF.type, DM.Metadata),
            graph.subjects(RDF.type, DM.Instance),
        )
        rdfid = URIRef(g.__next__())
        try:
            g.__next__()
        except StopIteration:
            pass
        else:
            raise ValueError(
                "id must be given when graph has move than one entity"
            )
    elif _is_valid_uri(id):
        rdfid = URIRef(id)
    else:
        rdfid = URIRef(PUBLIC_ID + id)

    # Get uuid and make sure that it is consistent with id
    uuid = graph.value(rdfid, DM.hasUUID)
    dlite_id = id if id else uuid
    dlite_id = dlite_id.split("#", 1)[-1]
    if not uuid:
        v = graph.value(None, DM.hasUUID, Literal(id))
        if v:
            rdfid = v
            uuid = id
            dlite_id = (
                rdfid.split("#", 1)[-1]
                if "#" in rdfid
                else rdfid.rsplit("/", 1)[-1]
            )
            rdfid = URIRef(rdfid)
    if uuid:
        uuid = str(uuid)

    if uuid:
        if dlite.get_uuid(dlite_id) != str(uuid):
            if dlite.get_uuid(dlite_id) != uuid:
                raise ValueError(
                    f'provided id "{id}" does not correspond '
                    f'to uuid "{uuid}"'
                )
    else:
        uuid = dlite.get_uuid(dlite_id)

    if dlite.has_instance(uuid):
        return dlite.get_instance(uuid)

    metaid = _value(graph, rdfid, DM.instanceOf)
    if dlite.has_instance(metaid):
        meta = dlite.get_instance(_value(graph, rdfid, DM.instanceOf))
    else:
        meta = from_graph(graph, id=metaid)
    dimensions = list(graph.objects(rdfid, DM.hasDimension))
    properties = list(graph.objects(rdfid, DM.hasProperty))

    if meta.is_metameta:
        dims = []
        for dim in dimensions:
            dims.append(
                dlite.Dimension(
                    name=_value(graph, dim, DM.hasLabel),
                    description=graph.value(dim, DM.hasDescription),
                )
            )
        props = []
        for prop in properties:
            dlite_shape = []
            shape = graph.value(prop, DM.hasShape)
            if shape:
                next = graph.value(shape, DM.hasFirst)
                while next:
                    dlite_shape.append(_value(graph, next, DM.hasValue))
                    next = graph.value(next, DM.hasNext)
            props.append(
                dlite.Property(
                    name=_value(graph, prop, DM.hasLabel),
                    type=_value(graph, prop, DM.hasType),
                    shape=dlite_shape,
                    unit=graph.value(prop, DM.hasUnit),
                    description=graph.value(prop, DM.hasDescription),
                )
            )
        inst = dlite.Metadata(
            uri=dlite_id,
            dimensions=dims,
            properties=props,
            description=graph.value(rdfid, DM.hasDescription),
        )
    else:
        dims = {
            str(_value(graph, dim, DM.hasLabel)): int(
                _value(graph, dim, DM.hasValue)
            )
            for dim in dimensions
        }
        inst = dlite.Instance.from_metaid(meta.uri, dims, id=dlite_id)
        for prop in properties:
            label = _value(graph, prop, DM.hasLabel)
            value = _value(graph, prop, DM.hasValue)
            inst.set_property_from_string(label, value, flags=1)

    return inst


def from_rdf(
    source=None,
    location=None,
    file=None,
    data=None,
    format=None,
    id=None,
    publicID=PUBLIC_ID,
    **kwargs,
):
    """Instantiate DLite instance from RDF.

    The source is specified using one of `source`, `location`, `file` or `data`.

    Arguments:
        source: An InputSource, file-like object, or string. In the case
            of a string the string is the location of the source.
        location: A string indicating the relative or absolute URL of the
            source. Graph's absolutize method is used if a relative location
            is specified.
        file: A file-like object.
        data: A string containing the data to be parsed.
        format: Used if format can not be determined from source, e.g. file
            extension or Media Type.  By default, the following formats are
            supported: "xml", "n3", "turtle", "nt" and "trix".
        id: Id of the instance to return.  May be None if the source only
            contain one instance.
        publicID: Logical URI to use as document base.
        kwargs:
            Additional keyword arguments passed to rdflib.Graph.parse().

    Returns
        New DLite instance.
    """
    graph = rdflib.Graph()
    if isinstance(source, pathlib.PurePath):
        source = str(source)
    if format is None:
        format = guess_format(source)
    graph.parse(
        source=source,
        location=location,
        file=file,
        data=data,
        format=format,
        publicID=publicID,
        **kwargs,
    )
    return from_graph(graph, id=id)
