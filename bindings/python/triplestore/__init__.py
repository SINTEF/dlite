"""A package encapsulating different triplestores using the strategy design
pattern.
"""
from typing import TYPE_CHECKING

from .triplestore import (
    Literal, Namespace, Triplestore,
    en,
    XML, RDF, RDFS, XSD, OWL, SKOS, DC, DCTERMS, FOAF, DOAP, FNO, EMMO, MAP, DM,
)

if TYPE_CHECKING:  # pragma: no cover
    from .triplestore import Triple


__all__ = (
    "Literal",
    "Namespace",
    "Triplestore",
    "en",
    "XML",
    "RDF",
    "RDFS",
    "XSD",
    "OWL",
    "SKOS",
    "DC",
    "DCTERMS",
    "FOAF",
    "DOAP",
    "FNO",
    "EMMO",
    "MAP",
    "DM",
)
