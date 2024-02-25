"""Module for representing DLite data models and instances with rdflib.

DLite data models are represented as EMMO datasets.
"""
import json
import warnings
from typing import TYPE_CHECKING

from tripper import Literal, Namespace, Triplestore
from tripper import MAP, OWL, RDF, RDFS, SKOS, XSD
from tripper.utils import en
from tripper.errors import NoSuchIRIError

import dlite

if TYPE_CHECKING:  # pragma: no cover
    from typings import List, Optional, Sequence, Tuple

    # A triple with literal objects in n3 notation
    Triple = Sequence[str, str, str]


OTEIO = Namespace("http://emmo.info/oteio.pipeline#")

# XXX TODO - Make a local cache of EMMO such that we only download it once
EMMO = Namespace(
    #iri="http://emmo.info/emmo#",
    iri="https://w3id.org/emmo#",
    label_annotations=True,
    #cachemode=Namespace.ONLY_CACHE,
    check=True,
    triplestore_url=(
        "https://raw.githubusercontent.com/emmo-repo/emmo-repo.github.io/"
        "master/versions/1.0.0-beta7/emmo-dataset.ttl"
    ),
)
description_iri = EMMO.elucidation  # TODO: deside what annotation to use


EMMO_TYPES = {
    "blob": "String",
    "bool": "Boolean",
    "int": "Integer",
    "uint": "Integer",
    "float": "Real",
    "fixstring": "String",
    "string": "String",
    "ref": "DataSet",
    "dimension": "Dimension",
    "property": "Datum",
    #"relation": NotImplemented,
}


def _string(s):
    """Return `s` as a literal string."""
    return Literal(s, datatype="xsd:string")


def title(s):
    """Capitalise first letter in `s`."""
    return s[0].upper() + s[1:]


def get_unit_iri(unit):
    """Returns the IRI for the given unit."""
    ts = EMMO._triplestore
    XXX - IMPLEMENT


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
    dct = meta.asdict()
    triples = []
    mapsto = {s: o for s, p, o in mappings if p == MAP.mapsTo}

    # Add datamodel (emmo:DataSet)
    if base_iri:
        uuid = meta.uuid.replace('-', '_')
        iri = f"{base_iri.rstrip('#/')}#{uuid_prefix}{uuid}"
    else:
        iri = meta.uri

    triples.extend([
        (iri, RDF.type, OWL.Class),
        (iri, RDFS.subClassOf, EMMO.DataSet),
        (iri, SKOS.prefLabel, en(title(meta.name))),
    ])

    if meta.uri in mapsto:
        triples.append((iri, RDFS.subClassOf, mapsto[meta.uri]))

    if "description" in dct:
        triples.append((iri, description_iri, en(dct["description"])))

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
        if prop_iri in mapsto:
            triples.append((prop_iri, RDFS.subClassOf, mapsto[prop_iri]))
        if emmotype:
            triples.append((prop_iri, RDFS.subClassOf, EMMO[emmotype]))
        if prop.unit:
            try:
                unit_iri = EMMO[prop.unit]
            except NoSuchIRIError:
                warnings.warn(f"unit '{prop.unit}' not found in EMMO")
                unit_iri = f":{prop.unit}"
            restriction_iri = f"_:restriction_{prop_iri}_unit"
            triples.extend([
                (prop_iri, RDFS.subclass, restriction_iri),
                (restriction_iri, RDF.type, OWL.Restriction),
                (restriction_iri, OWL.onProperty, EMMO.hasMeasurementUnit),
                (restriction_iri, OWL.onClass, unit_iri),
                (restriction_iri, OWL.qualifiedCardinality,
                 Literal(1, datatype=XSD.nonNegativeInteger)),
            ])
        if prop.shape.size:
            shape_id = f"{meta.uri}#{prop.name}Shape"
            if base_iri:
                uuid = dlite.get_uuid(shape_id).replace("-", "_")
                shape_iri = (f"{base_iri.rstrip('#/')}#{uuid_prefix}{uuid}")
            else:
                shape_iri = shape_id
            restriction_iri = f"_:restriction_{shape_iri}"
            triples.extend([
                (prop_iri, RDFS.subClassOf, EMMO.Array),
                (prop_iri, RDFS.subClassOf, restriction_iri),
                (restriction_iri, RDF.type, OWL.Restriction),
                (restriction_iri, OWL.onProperty, EMMO.hasShape),
                (restriction_iri, OWL.onClass, shape_iri),
                (restriction_iri, OWL.qualifiedCardinality,
                 Literal(1, datatype=XSD.nonNegativeInteger)),
                (shape_iri, RDF.type, OWL.Class),
                (shape_iri, RDFS.subclassOf, EMMO.Shape),
                (shape_iri, SKOS.prefLabel, en(f"{prop_name}Shape")),
                (shape_iri, description_iri,
                 en(f"Shape of datum '{prop_name}' of dataset '{meta.name}'.")),
            ])
            for i, dim in enumerate(prop.shape):
                dim_id = f"{meta.uri}#{prop.name}_dimension{i}"
                if base_iri:
                    uuid = dlite.get_uuid(dim_id).replace("-", "_")
                    dim_iri = (f"{base_iri.rstrip('#/')}#{uuid_prefix}{uuid}")
                else:
                    dim_iri = shape_id
                triples.extend([
                    (dim_iri, RDF.type, EMMO.Dimension),
                    (dim_iri, EMMO.hasSymbolValue,
                     Literal(dim, datatype=XSD.string)),
                    (dim_iri, description_iri, en(dim_descr[dim])),
                    (dim_iri, SKOS.prefLabel, en(f"{prop.name}_dimension{i}")),
                ])
                if i == 0:
                    restriction_iri = f"_:restriction_dimension{i}_{prop_iri}"
                    triples.extend([
                        (shape_iri, RDFS.subClassOf, restriction_iri),
                        (restriction_iri, RDF.type, OWL.Restriction),
                        (restriction_iri, OWL.onProperty, EMMO.hasBeginTile),
                        (restriction_iri, OWL.hasValue, dim_iri),
                    ])
                else:
                    triples.append((source_iri, EMMO.hasNext, dim_iri))
                source_iri = dim_iri



        #if prop.shape.size:
        #    shape_iri = f"_:shape_{prop_iri}"
        #    triples.extend([
        #        (prop_iri, RDF.type, EMMO.Array),
        #        (prop_iri, EMMO.hasShape, shape_iri),
        #        (shape_iri, RDF.type, EMMO.Shape),
        #        (shape_iri, RDF.type, RDF.List),
        #    ])
        #    source_iri = shape_iri
        #    for i, dim in enumerate(prop.shape):
        #        list_iri = f"_:shape{i}_{prop_iri}"
        #        dim_iri = f"_:dimension{i}_{prop_iri}"
        #        triples.extend([
        #            (source_iri, RDF.fir, RDF.List),
        #            (list_iri, RDF.type, RDF.List),
        #            (prop_iri, EMMO.hasShape, shape_iri),
        #            (shape_iri, RDF.type, EMMO.Shape),
        #            (shape_iri, RDF.type, RDF.List),
        #        ])
        #        source_iri = list_iri
        #    triples.append((source_iri, RDF.rest, RDF.nil))
        #
        #if prop.unit:
        #    restriction_iri = f"_:restriction_{prop_iri}_unit"
        #    triples.extend([
        #        (iri, RDFS.subClassOf, restriction_iri),
        #        (restriction_iri, RDF.type, OWL.Restriction),
        #        (restriction_iri, OWL.onProperty, EMMO.hasMeasurementUnit),
        #        (restriction_iri, OWL.onClass, EMMO[prop.unit]),
        #        (restriction_iri, OWL.qualifiedCardinality,
        #         Literal(1, datatype="xsd:nonNegativeInteger")),
        #    ])
        if prop.description:
            triples.append((prop_iri, description_iri, en(prop.description)))

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
                (inst.uri, description_iri, en(dct["description"]))
           )
        for prop in inst.properties["properties"]:
            iri = f"{inst.uri}#{prop.name}"
            restriction_iri = f"_:restriction_{prop.name}_{inst.uuid}"
            shape = json.dumps(prop.asdict().get("shape", []))

            triples.extend([
                (inst.uri, RDFS.subClassOf, restriction_iri),
                (restriction_iri, RDF.type, OWL.Restriction),
                (restriction_iri, OWL.onProperty, EMMO.hasDataEntry),
                (restriction_iri, OWL.onClass, iri),
                (restriction_iri, OWL.qualifiedCardinality,
                 Literal(1, datatype="xsd:nonNegativeInteger")),
                (iri, RDF.type, OWL.Class),
                (iri, RDFS.subClassOf, EMMO.Datum),
            ])
            if prop.shape.size:
                triples.append((iri, OTEIO.hasShape, Literal(shape)))
            if prop.description:
                triples.append((iri, description_iri, en(prop.description)))

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
