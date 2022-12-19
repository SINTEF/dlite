"""A package encapsulating different triplestores using the strategy design
pattern.

See the README.md file for a description for how to use this package.
"""
import warnings
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


warnings.warn(
    "dlite.triplestore is deprecated.\n"
    "Use tripper (https://github.com/EMMC-ASBL/tripper) instead.",
    DeprecationWarning,
    stacklevel=2,
)
