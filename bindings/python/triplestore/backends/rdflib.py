import warnings
from typing import TYPE_CHECKING

import rdflib
from rdflib import BNode, Graph, URIRef

from dlite.triplestore import Literal

if TYPE_CHECKING:  # pragma: no cover
    from collections.abc import Sequence
    from typing import Generator

    from dlite.triplestore import Triple


def asuri(v):
    """Help function converting a spo-value to proper rdflib type."""
    if v is None:
        return None
    if isinstance(v, Literal):
        return rdflib.Literal(v.value, lang=v.lang, datatype=v.datatype)
    if v.startswith("_:"):
        return BNode(v)
    return URIRef(v)


def astriple(t):
    """Help function converting a triple to rdflib triple."""
    s, p, o = t
    return asuri(s), asuri(p), asuri(o)


class RdflibStrategy:
    """Triplestore strategy for rdflib."""

    def __init__(self):
        self.graph = Graph()

    def triples(self, triple: "Triple") -> "Generator":
        """Returns a generator over matching triples."""
        for s, p, o in self.graph.triples(astriple(triple)):
            yield (str(s), str(p),
                   Literal(o.value, lang=o.language, datatype=o.datatype)
                   if isinstance(o, rdflib.Literal) else str(o))

    def add_triples(self, triples: "Sequence[Triple]"):
        """Add a sequence of triples."""
        for t in triples:
            self.graph.add(astriple(t))

    def remove(self, triple: "Triple"):
        """Remove triple from the backend."""
        self.graph.remove(astriple(triple))

    # Optional methods
    def parse(self, source=None, location=None, data=None, format=None,
              **kwargs):
        """Parse source and add the resulting triples to triplestore.

        The source is specified using one of `source`, `location` or `data`.

        Parameters:
            source: File-like object or file name.
            location: String with relative or absolute URL to source.
            data: String containing the data to be parsed.
            format: Needed if format can not be inferred from source.
            kwargs: Additional less used keyword arguments.
                See https://rdflib.readthedocs.io/en/stable/apidocs/rdflib.html#rdflib.Graph.parse
        """
        self.graph.parse(source=source, location=location, data=data,
                         format=format, **kwargs)

    def serialize(self, destination=None, format='turtle', **kwargs):
        """Serialise to destination.

        Parameters:
            destination: File name or object to write to.  If None, the
                serialisation is returned.
            format: Format to serialise as.  Supported formats, depends on
                the backend.
            kwargs: Passed to the rdflib.Graph.serialize() method.
                See https://rdflib.readthedocs.io/en/stable/apidocs/rdflib.html#rdflib.Graph.serialize

        Returns:
            Serialised string if `destination` is None.
        """
        s = self.graph.serialize(destination=destination, format=format,
                                 **kwargs)
        if destination is None:
            # Depending on the version of rdflib the return value of
            # graph.serialize() man either be a string or a bytes object...
            return s if isinstance(s, str) else s.decode()

    def query(self, query_object, **kwargs):
        """SPARQL query."""
        return self.graph.query(query_object=query_object, **kwargs)

    def update(self, update_object, **kwargs):
        """Update triplestore with SPARQL."""
        return self.graph.update(update_object=update_object, **kwargs)

    def bind(self, prefix: str, namespace: str):
        """Bind prefix to namespace.

        Should only be defined if the backend supports namespaces.
        Called by triplestore.bind().
        """
        if namespace:
            self.graph.bind(prefix, namespace, replace=True)
        else:
            warnings.warn(
                "rdflib does not support removing namespace prefixes")

    def namespaces(self) -> dict:
        """Returns a dict mapping prefixes to namespaces.

        Should only be defined if the backend supports namespaces.
        Used by triplestore.parse() to get prefixes after reading
        triples from an external source.
        """
        return {prefix: str(ns) for prefix, ns in self.graph.namespaces()}
