"""Module for representing DLite data models and instances with rdflib.

DLite data models are represented as EMMO datasets.
"""
import json
import warnings
from collections import defaultdict
from typing import TYPE_CHECKING

from tripper import Literal, Namespace, Triplestore
from tripper import MAP, OTEIO, OWL, RDF, RDFS, SKOS, XSD
from tripper.utils import en
from tripper.errors import NoSuchIRIError

import dlite

if TYPE_CHECKING:  # pragma: no cover
    from typings import List, Optional, Sequence, Tuple

    # A triple with literal objects in n3 notation
    Triple = Sequence[str, str, str]


#OTEIO = Namespace("http://emmo.info/oteio.pipeline#")

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
    "int": "IntegerData",
    "uint": "IntegerData",
    "float": "RealData",
    "fixstring": "StringData",
    "string": "StringData",
    "ref": "DataSet",
    "dimension": "Dimension",
    "property": "Datum",
    #"relation": NotImplemented,
}

# Maps unit names to IRIs
unit_cache = {}


class MissingUnitError(ValueError):
    "Unit not found in ontology."


def _string(s):
    """Return `s` as a literal string."""
    return Literal(s, datatype="xsd:string")


def title(s):
    """Capitalise first letter in `s`."""
    return s[0].upper() + s[1:]


def dimensional_string(unit_iri):
    """Return the inferred dimensional string of the given unit IRI.  Returns
    None if no dimensional string can be inferred."""
    ts = TS_EMMO


    for parent in ts.objects(iri, RDFS.subClassOf):
        pass


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
    mappings: "Sequence[Triple]" = (),
    base_iri: str = None,
    uuid_prefix: str = "EMMO_",
) -> "List[Triple]":
    """Serialise DLite metadata to RDF.

    Arguments:
        meta: Metadata to serialise.
        mappings: Sequence of mappings of properties to ontological concepts.
        base_iri: Base IRI to use for serialisation. Default is to use the
            metadata URI.
        uuid_prefix: Prefix to prepend to the UUID part of IRIs.

    Returns:
        A list of RDF triples.  Literal objects are encoded in n3 notation.
    """
    # Create lookup table for mappings
    dct = meta.asdict()

    maps = defaultdict(list)
    for s, p, o in mappings:
        maps[s].append((RDFS.subClassOf if p == MAP.mapsTo else p, o))

    #mapsto = {s: o for s, p, o in mappings if p == MAP.mapsTo}

    # Add datamodel (emmo:DataSet)
    if base_iri:
        uuid = meta.uuid.replace('-', '_')
        iri = f"{base_iri.rstrip('#/')}#{uuid_prefix}{uuid}"
    else:
        iri = meta.uri

    # Start populating triples
    triples = []
    triples.extend([
        (iri, RDF.type, OWL.Class),
        (iri, RDFS.subClassOf, EMMO.DataSet),
        (iri, SKOS.prefLabel, en(title(meta.name))),
    ])

    #if meta.uri in mapsto:
    #    triples.append((iri, RDFS.subClassOf, mapsto[meta.uri]))

    # Add additional relations from mappings
    for p, o in maps[meta.uri]:
        triples.append((iri, p, o))

    if "description" in dct:
        triples.append((iri, EMMO.elucidation, en(dct["description"])))

    dim_descr = {d.name: d.description for d in meta.properties['dimensions']}

    # Add properties (emmo:Datum)
    for prop in meta.properties["properties"]:
        prop_id = f"{meta.uri}#{prop.name}"
        if base_iri:
            uuid = dlite.get_uuid(prop_id).replace("-", "_")
            prop_iri = (f"{base_iri.rstrip('#/')}#{uuid_prefix}{uuid}")
        else:
            prop_iri = prop_id
        restriction_iri = f"_:restriction_{prop_iri}"
        emmotype = EMMO_TYPES.get(prop.type.rstrip("0123456789"))
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

        # Add additional relations from mappings
        for p, o in maps[prop_id]:
            triples.append((prop_iri, p, o))

        #if prop_iri in mapsto:
        #    triples.append((prop_iri, RDFS.subClassOf, mapsto[prop_iri]))

        if emmotype:
            triples.append((prop_iri, RDFS.subClassOf, EMMO[emmotype]))

        if prop.shape.size:
            restriction_iri = f"_:restriction_{prop_iri}_shape"
            triples.extend([
                (prop_iri, RDFS.subClassOf, EMMO.Array),
                (prop_iri, RDFS.subClassOf, restriction_iri),
                (restriction_iri, RDF.type, OWL.Restriction),
                (restriction_iri, OWL.onProperty, EMMO.hasDimension),
            ])
            for i, dim in enumerate(prop.shape):
                dim_id = f"{meta.uri}#{prop.name}_dimension{i}"
                if base_iri:
                    uuid = dlite.get_uuid(dim_id).replace("-", "_")
                    dim_iri = (f"{base_iri.rstrip('#/')}#{uuid_prefix}{uuid}")
                else:
                    dim_iri = f"{meta.uri}#{prop.name}_dimension"
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
    mappings: "Sequence[Triple]" = (),
    base_iri: str = None,
    uuid_prefix: str = "EMMO_",
) -> Triplestore:
    """Save DLite metadata as an EMMO dataset to a triplestore.

    Arguments:
        ts: Triplestore to save to.
        meta: DLite metadata to save.
        mappings: Sequence of mappings of properties to ontological concepts.
        base_iri: Base IRI to use for serialisation. Default is to use the
            metadata URI.
        uuid_prefix: Prefix to prepend to the UUID part of IRIs.
    """
    ts.add_triples(metadata_to_rdf(meta, mappings, base_iri=base_iri))
    if "emmo" not in ts.namespaces:
        ts.bind("emmo", EMMO)


def get_dataset(
    ts: Triplestore,
    iri: str,
) -> "Tuple[dlite.Metadata, List[Triple]]":
    """Load dataset from triplestore.

    Arguments:
        ts: Triplestore to save to.
        iri: IRI of the DLite metadata to load.

    Returns:
        A `(meta, mappings)` tuple, where `meta` is a DLite metadata and
        `mappings` is a list of triples.
    """
    poIri = dict(ts.predicate_objects(iri))





def to_rdf(
    inst: dlite.Instance,
    standard: str = "emmo",
    base_uri: str = "",
    include_meta: "Optional[bool]" = None,
) -> "List[Triple]":
    """Serialise DLite instance to RDF.

    Arguments:
        inst: Instance to serialise.
        standard: What standard to use when serialising `inst`.  Valid
            values are
              - "emmo": Serialise as EMMO DataSet.
              - "datamodel": According to the datamodel ontology.
        base_uri: Base URI that is prepended to the instance URI or UUID
            (if it is not already a valid URI).
        include_meta: Whether to also serialise metadata.  The default
            is to only include metadata if `inst` is a data object.

    Returns:
        A list of RDF triples.  Literal objects are encoded in n3 notation.
    """
    if standard == "emmo":
        return to_rdf_as_emmo(
            inst, base_uri=base_uri, include_meta=include_meta
        )
    elif standard == "datamodel":
        print("????????????????????????????????????????")
        from dlite.rdf import to_graph
        graph = to_graph(
            inst, base_uri=base_uri, include_meta=include_meta
        )
        return graph.serialize(format="n3")
    else:
        raise ValueError('`standard` must be either "emmo" or "datamodel".')


def to_rdf_as_emmo(
    inst: dlite.Instance,
    base_uri: str = "",
    include_meta: "Optional[bool]" = None,
) -> "List[Triple]":
    """Serialise DLite instance as an EMMO dataset.

    Arguments:
        inst: Instance to serialise.
        base_uri: Base URI that is prepended to the instance URI or UUID
            (if it is not already a valid URI).
        include_meta: Whether to also serialise metadata.  The default
            is to only include metadata if `inst` is a data object.

    Returns:
        A list of RDF triples.  Literal objects are encoded in n3 notation.
    """
    triples = []

    if include_meta is None:
        include_meta = not inst.is_meta
    if include_meta:
        # EMMO cannot represent metadata schema - use "datamodel" for that
        standard = "datamodel" if inst.is_meta else "emmo"
        triples.extend(
            to_rdf(
                inst=inst.meta,
                standard=standard,
                base_uri=base_uri,
                include_meta=False
            )
        )

    if inst.is_meta:
        dct = inst.asdict()
        dims = json.dumps(inst.asdict()["dimensions"])
        triples.extend([
            (inst.uri, RDF.type, OWL.Class),
            (inst.uri, RDFS.subClassOf, EMMO.DataSet),
            (inst.uri, OTEIO.hasDimension, Literal(dims)),
        ])
        if "description" in dct:
            triples.append(
                (inst.uri, EMMO.elucidation, en(dct["description"]))
           )
        for prop in inst.properties["properties"]:
            iri = f"{inst.uri}#{prop.name}"
            restriction_iri = f"_:restriction_{prop.name}_{inst.uuid}"
            shape = json.dumps(prop.asdict().get("shape", []))

            triples.extend([
                (inst.uri, RDFS.subClassOf, restriction_iri),
                (restriction_iri, RDF.type, OWL.Restriction),
                (restriction_iri, OWL.onProperty, EMMO.hasDatum),
                (restriction_iri, OWL.onClass, iri),
                (restriction_iri, OWL.qualifiedCardinality,
                 Literal(1, datatype="xsd:nonNegativeInteger")),
                (iri, RDF.type, OWL.Class),
                (iri, RDFS.subClassOf, EMMO.Datum),
            ])
            if prop.shape.size:
                triples.append((iri, OTEIO.hasShape, Literal(shape)))
            if prop.description:
                triples.append((iri, EMMO.elucidation, en(prop.description)))

            typeno, size = dlite.from_typename(prop.type)
            typename = dlite.to_typename(typeno, size)
            stripname = typename.rstrip("0123456789")
            emmo_type = "Array" if prop.ndims else EMMO_TYPE[stripname]
            if emmo_type is not NotImplemented:
                triples.append((iri, RDFS.subClassOf, EMMO[emmo_type]))

            if prop.unit:
                restriction_iri = f"_:restriction_{prop.name}_unit_{inst.uuid}"
                triples.extend([
                    (iri, RDFS.subClassOf, restriction_iri),
                    (restriction_iri, RDF.type, OWL.Restriction),
                    (restriction_iri, OWL.onProperty, EMMO.hasMeasurementUnit),
                    (restriction_iri, OWL.onClass, EMMO[prop.unit]),  # XXX
                    (restriction_iri, OWL.qualifiedCardinality,
                     Literal(1, datatype="xsd:nonNegativeInteger")),
                ])

    return triples
