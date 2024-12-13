"""Module for representing DLite data models and instances with rdflib.

DLite data models are represented as EMMO datasets.

NOTE: This module depends on Tripper.
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
TS_EMMO.parse("https://w3id.org/emmo/1.0.0-rc3")

EMMO_VERSIONIRI = TS_EMMO.value("https://w3id.org/emmo", OWL.versionIRI)

EMMO = Namespace(
    iri="https://w3id.org/emmo#",
    label_annotations=True,
    check=True,
    triplestore=TS_EMMO,
)

# XXX TODO: Switch to EMMO.hasDimension when this relation is in EMMO.
#           Please don't change the IRI when adding it.
#Dimension = EMMO.Dimension
Dimension = "https://w3id.org/emmo#EMMO_b4c97fa0_d82c_406a_bda7_597d6e190654"
#hasDimension = EMMO.hasDimension
hasDimension = "https://w3id.org/emmo#EMMO_0a9ae0cb_526d_4377_9a11_63d1ce5b3499"
#hasScalarData = EMMO.hasScalarData
hasScalarData = "https://w3id.org/emmo#EMMO_e5a34647_a955_40bc_8d81_9b784f0ac527"

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
            elif pred == RDF.type and obj not in (Dimension,):
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
    for r in TS_EMMO.restrictions(iri, EMMO.unitSymbolValue, type="value"):
        symbol = r["value"]
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
                    ts.has(r, OWL.onProperty, EMMO.unitSymbolValue)
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
        (iri, OTEIO.hasURI, Literal(meta.uri, datatype=XSD.anyURI)),
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
        ])

        emmotype, size = dlite2emmotype(prop.type)
        if prop.ndims:
            restriction_iri = f"_:restriction_type_{prop_iri}"
            triples.extend([
                (prop_iri, RDFS.subClassOf, restriction_iri),
                (restriction_iri, RDF.type, OWL.Restriction),
                (restriction_iri, OWL.onProperty, hasScalarData),
                (restriction_iri, OWL.someValuesFrom, EMMO[emmotype]),
            ])
        else:
            triples.append((prop_iri, RDFS.subClassOf, EMMO[emmotype]))
        if size:
            sizeval = Literal(size, datatype=XSD.nonNegativeInteger)
            triples.append((prop_iri, OTEIO.datasize, sizeval))

        if prop.shape.size:
            restriction_iri = f"_:restriction_{prop_iri}_shape"
            triples.extend([
                (prop_iri, RDFS.subClassOf, EMMO.Array),
                (prop_iri, RDFS.subClassOf, restriction_iri),
                (restriction_iri, RDF.type, OWL.Restriction),
                (restriction_iri, OWL.onProperty, hasDimension),
            ])
            for i, dim in enumerate(prop.shape):
                dim_iri = f"{iri}#{prop.name}_dimension{i}"
                addmap(f"{meta.uri}#{dim}", dim_iri)
                triples.extend([
                    (dim_iri, RDF.type, Dimension),
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
    if not meta.is_meta:
        raise TypeError(
            "Expected data model, got instance: {meta.uri or meta.uuid}"
        )

    if iri is None:
        iri = meta.uri
    iri = str(iri).rstrip("#/")

    ts.add_triples(metadata_to_rdf(meta, iri=iri, mappings=mappings))

    used_namespaces = {"emmo": EMMO, "oteio": OTEIO}
    for prefix, ns in used_namespaces.items():
        if prefix not in ts.namespaces:
            ts.bind(prefix, ns)

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
        uri: URI of the DLite datamodel to load. The defaults is inferred
            from `iri`.

    Returns:
        A `(meta, mappings)` tuple, where `meta` is a DLite metadata and
        `mappings` is a list of triples.
    """
    if uri is None:
        uri = str(ts.value(iri, OTEIO.hasURI, default=str(iri).rstrip("/#")))

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
                        elif onprop == hasScalarData:
                            emmotype = emmotypes[someval]
                        elif onprop == hasDimension:
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


def add_data(
    ts: Triplestore,
    inst: dlite.Instance,
    iri: "Optional[str]" = None,
    mappings: "Sequence[Triple]" = (),
) -> str:
    """Save DLite instance as an EMMO dataset to a triplestore.

    Data instances are represented as individuals of the corresponding
    EMMO DataSet. The corresponding metadata is also stored if it not
    already exists in the triplestore.

    Arguments:
        ts: Triplestore to save to.
        inst: DLite instance to save.
        iri: IRI of the dataset in the triplestore. The default is the
            metadata IRI prepended with a slash and the UUID.
        mappings: Sequence of mappings of properties to ontological concepts.

    Returns:
        IRI of the saved dataset.
    """
    if inst.is_meta:
        return add_dataset(ts, inst, iri, mappings)

    metairi = ts.value(
        predicate=OTEIO.hasURI,
        object=Literal(inst.meta.uri, datatype=XSD.anyURI),
    )
    if not metairi:
        metairi = add_dataset(ts, inst.meta)

    if not iri:
        iri = f"{metairi}/{inst.uri or inst.uuid}"

    triples = []
    triples.extend([
        (iri, RDF.type, metairi),
        (iri, OTEIO.hasUUID, Literal(inst.uuid, datatype=XSD.string)),
        (iri, RDF.value, Literal(inst.asjson(), datatype=RDF.JSON)),
    ])
    if inst.uri:
        triples.append(
            (iri, OTEIO.hasURI, Literal(inst.uri, datatype=XSD.string))
        )
    ts.add_triples(triples)

    used_namespaces = {"oteio": OTEIO}
    for prefix, ns in used_namespaces.items():
        if prefix not in ts.namespaces:
            ts.bind(prefix, ns)

    return iri


def get_data(
    ts: Triplestore,
    iri: str,
) -> "Tuple[dlite.Metadata, List[Triple]]":
    """Load dataset from triplestore.

    Arguments:
        ts: Triplestore to load from.
        iri: IRI of the dataset to load.

    Returns:
        A `(meta, mappings)` tuple, where `meta` is a DLite metadata and
        `mappings` is a list of triples.
    """
    mappings = []

    # Bypass the triplestore if the instance is already in cache
    try:
        return dlite.get_instance(iri, check_storages=False), mappings
    except dlite.DLiteMissingInstanceError:
        pass

    metairi = ts.value(iri, RDF.type, default=None)

    if not metairi:
        # `iri` does not correspond to a data instance, check for metadata
        if ts.has(iri, RDFS.subClassOf, EMMO.DataSet):
            return get_dataset(ts, iri), mappings
        raise KBError(
            f"Cannot find neither a data instance nor metadata with IRI: {iri}"
        )

    if not dlite.has_instance(metairi, check_storages=False):
        meta, maps = get_dataset(ts, metairi)
        mappings.extend(maps)
    else:
        meta = dlite.get_instance(metairi, check_storages=False)

    json = ts.value(iri, RDF.value)
    if not json:
        raise KBError(f"cannot find JSON value for IRI: {iri}")

    inst = dlite.Instance.from_json(str(json))

    return inst, mappings
