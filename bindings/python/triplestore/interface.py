"""Triplestore interface."""
from typing import TYPE_CHECKING, Protocol

if TYPE_CHECKING:  # pragma: no cover
    from collections.abc import Sequence
    from typing import Generator

    from dlite.triplestore import Triple


class ITriplestore(Protocol):
    """Interface for triplestore backends.

    In addition to the methods specified by this interface, a backend
    may also implement the following optional methods:

    ```
    serialize(self, destination=None, format='xml', **kwargs)
    query(self, query_object, **kwargs)
    update(self, update_object, **kwargs)
    bind(self, prefix: str, namespace: str)
    namespaces(self) -> dict
    ```
    """

    def triples(self, triple: "Triple") -> "Generator":
        """Returns a generator over matching triples."""

    def add_triples(self, triples: "Sequence[Triple]"):
        """Add a sequence of triples."""

    def remove(self, triple: "Triple"):
        """Remove all matching triples from the backend."""
