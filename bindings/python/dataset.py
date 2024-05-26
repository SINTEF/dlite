"""Module for representing DLite data models and instances with rdflib.

DLite data models are represented as EMMO datasets.
"""
import json
import re
import warnings
from collections import defaultdict
from typing import TYPE_CHECKING
from uuid import uuid4

from tripper import Literal, Namespace, Triplestore
from tripper import MAP, OTEIO, OWL, RDF, RDFS, SKOS, XSD
from tripper.utils import en
from tripper.errors import NoSuchIRIError

import dlite

if TYPE_CHECKING:  # pragma: no cover
    from typings import List, Optional, Sequence, Tuple

    # A triple with literal objects in n3 notation
    Triple = Sequence[str, str, str]


# XXX TODO - Make a local cache of EMMO such that we only download it once
TS_EMMO = Triplestore("rdflib")
TS_EMMO.parse("https://w3id.org/emmo/1.0.0-rc1")

EMMO_VERSIONIRI = TS_EMMO.value("https://w3id.org/emmo", OWL.versionIRI)

EMMO = Namespace(
    iri="https://w3id.org/emmo#",
    label_annotations=True,
    check=True,
    triplestore=TS_EMMO,
)

EMMO_TYPES = {
    "blob": "BinaryData",
    "bool": "BooleanData",
    "int":  "IntegerData",
    "int8": "ByteData",
    "int16": "ShortData",
    "int32": "IntData",
    "int64": "LongData",
    "uint": "NonNegativeIntegerData",
    "uint8": "UnsignedByteData",
    "uint16": "UnsignedShortData",
    "uint32": "UnsignedIntData",
    "uint64": "UnsignedLongData",
    "float": "FloatingPointData",
    "float32": "FloatData",
    "float64": "DoubleData",
    "string": "StringData",
    "ref": "DataSet",
    #"dimension": "Dimension",
    #"property": "Datum",
    #"relation": NotImplemented,
}

# Maps unit names to IRIs
unit_cache = {}


class MissingUnitError(ValueError):
    "Unit not found in ontology."

class UnsupportedTypeError(TypeError, NotImplementedError):
    "The given type is not supported."

class KBError(ValueError):
    "Missing or inconsistent data in knowledge base."


def _string(s):
    """Return `s` as a literal string."""
    return Literal(s, datatype="xsd:string")


def title(s):
    """Capitalise first letter in `s`."""
    return s[0].upper() + s[1:]


def dlite2emmotype(dlitetype):
    """Convert a DLite type string to corresponding EMMO class label."""
    dtype, ssize = re.match("([a-zA-Z]+)([0-9]*)", dlitetype).groups()
    size = int(ssize) if ssize else None
    if size and dtype in ("int", "uint", "float"):
        size /= 8
    if dlitetype in EMMO_TYPES:
        emmotype = EMMO_TYPES[dlitetype]
    elif dtype in EMMO_TYPES:
        emmotype = EMMO_TYPES[dtype]
    else:
        raise UnsupportedTypeError(dlitetype)
    return emmotype, size


def emmo2dlitetype(emmotype, size=None):
    """Convert EMMO type and size to dlite type."""
    dlitetypes = [k for k, v in EMMO_TYPES.items() if v == emmotype]
    if not dlitetypes:
        raise UnsupportedTypeError(emmotype)
    dlitetype, = dlitetypes
    typeno = dlite.to_typenumber(dlitetype.rstrip("0123456789"))
    if size:
        return dlite.to_typename(typeno, int(size))
    return dlite.to_typename(typeno)


def get_shape(ts, dimiri, dimensions=None, mappings=None, uri=None):
    """Returns a shape list for a datum who's first dimension is `dimiri`.

    If `dimensions` is given, it should be a list that will be updated
    with new dimensions.

    If `mappings` and `uri` are given, then `mappings` should be a
    list that will be updated with new mappings. `uri` should be the
    URI of the data model.
    """
    shape = []
    while dimiri:
        mapsto = []
        next = label = descr = None
        for pred, obj in ts.predicate_objects(dimiri):
            if pred == EMMO.hasNext:
                next = obj
            elif pred == EMMO.hasSymbolValue:
                label = str(obj)
            elif pred == EMMO.elucidation:
                descr = str(obj)
            elif pred == RDF.type and obj not in (EMMO.Dimension,):
                mapsto.append(obj)
        if not label:
            raise KBError("dimension has no prefLabel:", dimiri)
        if dimensions is not None:
            if not descr:
                raise KBError("dimension has no elucidation:", dimiri)
            dimensions.append(dlite.Dimension(label, descr))
        shape.append(label)
        if mappings and uri:
            for obj in mapsto:
                mappings.append((f"{uri}#{label}", MAP.mapsTo, obj))
        dimiri = next
    return shape


def dimensional_string(unit_iri):
    """Return the inferred dimensional string of the given unit IRI.  Returns
    None if no dimensional string can be inferred."""
    raise NotImplementedError()


def get_unit_symbol(iri):
    """Return the unit symbol for ."""
    symbol = TS_EMMO.value(iri, EMMO.unitSymbol)
    if symbol:
        return str(symbol)
    for r in TS_EMMO.restrictions(iri, EMMO.hasSymbolValue, type="value"):
        symbol = TS_EMMO.value(r, OWL.hasValue)
        if symbol:
            return str(symbol)
    raise KBError("No symbol value is defined for unit:", iri)


def get_unit_iri(unit):
    """Returns the IRI for the given unit."""
    if not unit_cache:
        ts = TS_EMMO
        for predicate in (EMMO.unitSymbol, EMMO.ucumCode, EMMO.uneceCommonCode):
            for s, _, o in ts.triples(predicate=predicate):
                if o.value in unit_cache and predicate == EMMO.unitSymbol:
                    warnings.warn(
                        f"more than one unit with symbol '{o.value}': "
                        f"{unit_cache[o.value]}"
                    )
                else:
                    unit_cache[o.value] = s
                for o in ts.objects(s, SKOS.prefLabel):
                    unit_cache[o.value] = s
                for o in ts.objects(s, SKOS.altLabel):
                    if o.value not in unit_cache:
                        unit_cache[o.value] = s

        for r, _, o in ts.triples(predicate=OWL.hasValue):
            if (
                    ts.has(r, RDF.type, OWL.Restriction) and
                    ts.has(r, OWL.onProperty, EMMO.hasSymbolValue)
            ):
                s = ts.value(predicate=RDFS.subClassOf, object=r)
                unit_cache[o.value] = s

    if unit in unit_cache:
        return unit_cache[unit]

    raise MissingUnitError(unit)


def metadata_to_rdf(
    meta: dlite.Metadata,
    iri: "Optional[str]" = None,
    mappings: "Sequence[Triple]" = (),
) -> "List[Triple]":
    """Serialise DLite metadata to RDF.

    Arguments:
        meta: Metadata to serialise.
        iri: IRI of the dataset in the triplestore. Defaults to `meta.uri`.
        mappings: Sequence of mappings of properties to ontological concepts.

    Returns:
        A list of RDF triples.  Literal objects are encoded in n3 notation.
    """
    # Create lookup table
    dct = meta.asdict()

    # For adding mappings
    maps = defaultdict(list)
    for s, p, o in mappings:
        uri = str(s).rstrip("/#")
        if p == MAP.mapsTo:
            name = str(s).split("#", 1)[-1]
            prep = RDF.type if name in meta.dimnames() else RDFS.subClassOf
        else:
            prep = p
        maps[uri].append((prep, o))

    def addmap(uri, iri):
        """Add mapping relation to triples."""
        for p, o in maps[uri.rstrip("/#")]:
            if p in (RDF.type, RDFS.subClassOf):
                triples.append((iri, p, o))
            else:
                restriction_iri = f"_:restriction_map_{iri}_{uuid4()}"
                triples.extend([
                    (iri, RDFS.subClassOf, restriction_iri),
                    (restriction_iri, RDF.type, OWL.Restriction),
                    (restriction_iri, OWL.onProperty, p),
                    (restriction_iri, OWL.someValuesFrom, o),
                ])

    # Dimension descriptions
    dim_descr = {d.name: d.description for d in meta.properties['dimensions']}

    # Start populating triples
    triples = []

    # Add datamodel (emmo:DataSet)
    if iri is None:
        iri = meta.uri
    iri = str(iri).rstrip("#/")
    triples.extend([
        (iri, RDF.type, OWL.Class),
        (iri, RDFS.subClassOf, EMMO.DataSet),
        (iri, SKOS.prefLabel, en(title(meta.name))),
        (iri, OTEIO.hasURI, meta.uri),
    ])
    addmap(meta.uri, iri)

    if "description" in dct:
        triples.append((iri, EMMO.elucidation, en(dct["description"])))

    # Add properties (emmo:Datum)
    for prop in meta.properties["properties"]:
        prop_id = f"{meta.uri}#{prop.name}"
        prop_iri = f"{iri}#{prop.name}"
        addmap(prop_id, prop_iri)
        restriction_iri = f"_:restriction_{prop_iri}"
        emmotype, size = dlite2emmotype(prop.type)
        prop_name = f"{prop.name[0].upper()}{prop.name[1:]}"
        triples.extend([
            (iri, RDFS.subClassOf, restriction_iri),
            (restriction_iri, RDF.type, OWL.Restriction),
            (restriction_iri, OWL.onProperty, EMMO.hasDatum),
            (restriction_iri, OWL.onClass, prop_iri),
            (restriction_iri, OWL.qualifiedCardinality,
             Literal(1, datatype=XSD.nonNegativeInteger)),
            (prop_iri, RDF.type, OWL.Class),
            (prop_iri, RDFS.subClassOf, EMMO.Datum),
            (prop_iri, SKOS.prefLabel, en(prop_name)),
            (prop_iri, RDFS.subClassOf, EMMO[emmotype]),
        ])
        if size:
            sizeval = Literal(size, datatype=XSD.nonNegativeInteger)
            triples.append((prop_iri, OTEIO.datasize, sizeval))

        if prop.shape.size:
            restriction_iri = f"_:restriction_{prop_iri}_shape"
            triples.extend([
                (prop_iri, RDFS.subClassOf, EMMO.Array),
                (prop_iri, RDFS.subClassOf, restriction_iri),
                (restriction_iri, RDF.type, OWL.Restriction),
                (restriction_iri, OWL.onProperty, EMMO.hasDimension),
            ])
            for i, dim in enumerate(prop.shape):
                dim_iri = f"{iri}#{prop.name}_dimension{i}"
                addmap(f"{meta.uri}#{dim}", dim_iri)
                triples.extend([
                    (dim_iri, RDF.type, EMMO.Dimension),
                    (dim_iri, EMMO.hasSymbolValue,
                     Literal(dim, datatype=XSD.string)),
                    (dim_iri, EMMO.elucidation, en(dim_descr[dim])),
                    (dim_iri, SKOS.prefLabel, en(f"{prop.name}_dimension{i}")),
                ])
                if i == 0:
                    triples.append((restriction_iri, OWL.hasValue, dim_iri))
                else:
                    triples.append((source_iri, EMMO.hasNext, dim_iri))
                source_iri = dim_iri

        if prop.unit:
            unit_iri = get_unit_iri(prop.unit)
            if unit_iri:
                restriction_iri = f"_:restriction_{prop_iri}_unit"
                triples.extend([
                    (prop_iri, RDFS.subClassOf, restriction_iri),
                    (restriction_iri, RDF.type, OWL.Restriction),
                    (restriction_iri, OWL.onProperty, EMMO.hasMeasurementUnit),
                    (restriction_iri, OWL.onClass, unit_iri),
                    (restriction_iri, OWL.qualifiedCardinality,
                     Literal(1, datatype=XSD.nonNegativeInteger)),
                ])

        if prop.description:
            triples.append((prop_iri, EMMO.elucidation, en(prop.description)))

    return triples


def add_dataset(
    ts: Triplestore,
    meta: dlite.Metadata,
    iri: "Optional[str]" = None,
    mappings: "Sequence[Triple]" = (),
) -> str:
    """Save DLite metadata as an EMMO dataset to a triplestore.

    Arguments:
        ts: Triplestore to save to.
        meta: DLite metadata to save.
        iri: IRI of the dataset in the triplestore. Defaults to `meta.uri`.
        mappings: Sequence of mappings of properties to ontological concepts.

    Returns:
        IRI of the saved dataset.
    """
    if iri is None:
        iri = meta.uri
    iri = str(iri).rstrip("#/")

    ts.add_triples(metadata_to_rdf(meta, iri=iri, mappings=mappings))
    if "emmo" not in ts.namespaces:
        ts.bind("emmo", EMMO)

    return iri


def get_dataset(
    ts: Triplestore,
    iri: str,
    uri: "Optional[str]" = None,
) -> "Tuple[dlite.Metadata, List[Triple]]":
    """Load dataset from triplestore.

    Arguments:
        ts: Triplestore to load from.
        iri: IRI of the dataset to load.
        uri: URI of datamodel to load. Defaults to `iri`.

    Returns:
        A `(meta, mappings)` tuple, where `meta` is a DLite metadata and
        `mappings` is a list of triples.
    """
    if uri is None:
        uri = ts.value(iri, OTEIO.hasURI, default=str(iri).rstrip("/#"))

    emmotypes = {EMMO[v]: v for v in EMMO_TYPES.values()}

    mappings = []
    dimensions = []
    properties = []
    description = ""
    datum_iris = []

    for prop, obj in ts.predicate_objects(iri):
        if prop == RDFS.subClassOf:
            po = set(ts.predicate_objects(obj))
            if (RDF.type, OWL.Restriction) in po:
                d = dict(po)
                onprop = d.get(OWL.onProperty)
                oncls = d.get(OWL.onClass)
                someval = d.get(OWL.someValuesFrom)
                if (OWL.onProperty, EMMO.hasDatum) in po:
                    datum_iris.append(oncls or someval)
                elif onprop and oncls:
                    mappings.append((uri, onprop, oncls))
                elif onprop and someval:
                    mappings.append((uri, onprop, someval))
            elif obj not in (EMMO.DataSet, ):
                mappings.append((uri, MAP.mapsTo, obj))
        elif prop == EMMO.elucidation:
            description = str(obj)

    for datum_iri in datum_iris:
        label = emmotype = size = None
        unit = descr = ""
        shape = []
        maps = []
        for pred, obj in ts.predicate_objects(datum_iri):
            if pred == SKOS.prefLabel:
                label = str(obj)
            elif pred == EMMO.elucidation:
                descr = str(obj)
            elif pred == OTEIO.datasize:
                size = int(obj)
            elif RDFS.subClassOf:
                if obj in emmotypes:
                    emmotype = emmotypes[obj]
                else:
                    po = dict(ts.predicate_objects(obj))
                    if po.get(RDF.type) == OWL.Restriction:
                        onprop = po.get(OWL.onProperty)
                        oncls = po.get(OWL.onClass)
                        onval = po.get(OWL.hasValue)
                        someval = po.get(OWL.someValuesFrom)
                        if onprop == EMMO.hasMeasurementUnit:
                            unit = get_unit_symbol(oncls)
                        elif onprop == EMMO.hasDimension:
                            shape = get_shape(
                                ts, onval, dimensions, mappings, uri
                            )
                        else:
                            maps.append((onprop, oncls or someval))
                    else:
                        maps.append((MAP.mapsTo, obj))
        if not label:
            raise KBError("missing preferred label on datum:", datum_iri)
        if not emmotype:
            raise KBError("missing type on datum:", datum_iri)
        for pred, obj in maps:
            if pred and obj and obj not in (OWL.Class, EMMO.Datum, EMMO.Array):
                mappings.append((f"{uri}#{label}", pred, obj))

        dlitetype = emmo2dlitetype(emmotype, size)
        properties.append(dlite.Property(
            name=label, type=dlitetype, shape=shape, unit=unit, description=descr))

    meta = dlite.Metadata(uri, dimensions, properties, description)

    return meta, mappings


#def to_rdf(
#    inst: dlite.Instance,
#    standard: str = "emmo",
#    base_uri: str = "",
#    include_meta: "Optional[bool]" = None,
#) -> "List[Triple]":
#    """Serialise DLite instance to RDF.
#
#    Arguments:
#        inst: Instance to serialise.
#        standard: What standard to use when serialising `inst`.  Valid
#            values are
#              - "emmo": Serialise as EMMO DataSet.
#              - "datamodel": According to the datamodel ontology.
#        base_uri: Base URI that is prepended to the instance URI or UUID
#            (if it is not already a valid URI).
#        include_meta: Whether to also serialise metadata.  The default
#            is to only include metadata if `inst` is a data object.
#
#    Returns:
#        A list of RDF triples.  Literal objects are encoded in n3 notation.
#    """
#    if standard == "emmo":
#        return to_rdf_as_emmo(
#            inst, base_uri=base_uri, include_meta=include_meta
#        )
#    elif standard == "datamodel":
#        print("????????????????????????????????????????")
#        from dlite.rdf import to_graph
#        graph = to_graph(
#            inst, base_uri=base_uri, include_meta=include_meta
#        )
#        return graph.serialize(format="n3")
#    else:
#        raise ValueError('`standard` must be either "emmo" or "datamodel".')
#
#
#def to_rdf_as_emmo(
#    inst: dlite.Instance,
#    base_uri: str = "",
#    include_meta: "Optional[bool]" = None,
#) -> "List[Triple]":
#    """Serialise DLite instance as an EMMO dataset.
#
#    Arguments:
#        inst: Instance to serialise.
#        base_uri: Base URI that is prepended to the instance URI or UUID
#            (if it is not already a valid URI).
#        include_meta: Whether to also serialise metadata.  The default
#            is to only include metadata if `inst` is a data object.
#
#    Returns:
#        A list of RDF triples.  Literal objects are encoded in n3 notation.
#    """
#    triples = []
#
#    if include_meta is None:
#        include_meta = not inst.is_meta
#    if include_meta:
#        # EMMO cannot represent metadata schema - use "datamodel" for that
#        standard = "datamodel" if inst.is_meta else "emmo"
#        triples.extend(
#            to_rdf(
#                inst=inst.meta,
#                standard=standard,
#                base_uri=base_uri,
#                include_meta=False
#            )
#        )
#
#    if inst.is_meta:
#        dct = inst.asdict()
#        dims = json.dumps(inst.asdict()["dimensions"])
#        triples.extend([
#            (inst.uri, RDF.type, OWL.Class),
#            (inst.uri, RDFS.subClassOf, EMMO.DataSet),
#            (inst.uri, OTEIO.hasDimension, Literal(dims)),
#        ])
#        if "description" in dct:
#            triples.append(
#                (inst.uri, EMMO.elucidation, en(dct["description"]))
#           )
#        for prop in inst.properties["properties"]:
#            iri = f"{inst.uri}#{prop.name}"
#            restriction_iri = f"_:restriction_{prop.name}_{inst.uuid}"
#            shape = json.dumps(prop.asdict().get("shape", []))
#
#            triples.extend([
#                (inst.uri, RDFS.subClassOf, restriction_iri),
#                (restriction_iri, RDF.type, OWL.Restriction),
#                (restriction_iri, OWL.onProperty, EMMO.hasDatum),
#                (restriction_iri, OWL.onClass, iri),
#                (restriction_iri, OWL.qualifiedCardinality,
#                 Literal(1, datatype="xsd:nonNegativeInteger")),
#                (iri, RDF.type, OWL.Class),
#                (iri, RDFS.subClassOf, EMMO.Datum),
#            ])
#            if prop.shape.size:
#                triples.append((iri, OTEIO.hasShape, Literal(shape)))
#            if prop.description:
#                triples.append((iri, EMMO.elucidation, en(prop.description)))
#
#            typeno, size = dlite.from_typename(prop.type)
#            typename = dlite.to_typename(typeno, size)
#            stripname = typename.rstrip("0123456789")
#            emmo_type = "Array" if prop.ndims else EMMO_TYPE[stripname]
#            if emmo_type is not NotImplemented:
#                triples.append((iri, RDFS.subClassOf, EMMO[emmo_type]))
#
#            if prop.unit:
#                restriction_iri = f"_:restriction_{prop.name}_unit_{inst.uuid}"
#                triples.extend([
#                    (iri, RDFS.subClassOf, restriction_iri),
#                    (restriction_iri, RDF.type, OWL.Restriction),
#                    (restriction_iri, OWL.onProperty, EMMO.hasMeasurementUnit),
#                    (restriction_iri, OWL.onClass, EMMO[prop.unit]),  # XXX
#                    (restriction_iri, OWL.qualifiedCardinality,
#                     Literal(1, datatype="xsd:nonNegativeInteger")),
#                ])
#
#    return triples
#
