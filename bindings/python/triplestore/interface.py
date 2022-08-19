"""Provides the ITriplestore protocol class, that documents the interface
of the triplestore backends."""
from typing import TYPE_CHECKING, Protocol

if TYPE_CHECKING:  # pragma: no cover
    from collections.abc import Sequence
    from typing import Generator

    from dlite.triplestore import Triple


class ITriplestore(Protocol):
    '''Interface for triplestore backends.

    In addition to the methods specified by this interface, a backend
    may also implement the following optional methods:

    ```python

    def parse(self, source=None, location=None, data=None, format=None,
              **kwargs):
        """Parse source and add the resulting triples to triplestore.

        The source is specified using one of `source`, `location` or `data`.

        Parameters:
            source: File-like object or file name.
            location: String with relative or absolute URL to source.
            data: String containing the data to be parsed.
            format: Needed if format can not be inferred from source.
            kwargs: Additional backend-specific parameters controlling
                the parsing.
        """

    def serialize(self, destination=None, format='xml', **kwargs)
        """Serialise to destination.

        Parameters:
            destination: File name or object to write to.  If None, the
                serialisation is returned.
            format: Format to serialise as.  Supported formats, depends on
                the backend.
            kwargs: Additional backend-specific parameters controlling
                the serialisation.

        Returns:
            Serialised string if `destination` is None.
        """

    def query(self, query_object, **kwargs)
        """SPARQL query."""

    def update(self, update_object, **kwargs)
        """Update triplestore with SPARQL."""

    def bind(self, prefix: str, namespace: str)
        """Bind prefix to namespace.

        Should only be defined if the backend supports namespaces.
        """

    def namespaces(self) -> dict
        """Returns a dict mapping prefixes to namespaces.

        Should only be defined if the backend supports namespaces.
        Used by triplestore.parse() to get prefixes after reading
        triples from an external source.
        """
    ```
    '''

    def triples(self, triple: "Triple") -> "Generator":
        """Returns a generator over matching triples."""

    def add_triples(self, triples: "Sequence[Triple]"):
        """Add a sequence of triples."""

    def remove(self, triple: "Triple"):
        """Remove all matching triples from the backend."""
