"""DLite Python storage plugin for RDF."""
from typing import TYPE_CHECKING

import rdflib
from rdflib.util import guess_format

import dlite
from dlite.options import Options
from dlite.rdf import DM, PUBLIC_ID, from_graph, from_rdf, to_graph, to_rdf

if TYPE_CHECKING:  # pragma: no cover
    from typing import Generator, Optional


class pyrdf(dlite.DLiteStorageBase):
    """DLite storage plugin for RDF serialisation."""

    def open(self, location: str, options: "Optional[str]" = None) -> None:
        """Opens `location`.

        Parameters:
            location: A fully resolved URL to the RDF source.
            options: Supported options:

            * `mode`: Mode for opening.
              Valid values are:

              - `a`: Append to existing file or create new file (default).
              - `r`: Open existing file for read-only.
              - `w`: Truncate existing file or create new file.

            * `format`: File format. For a complete list of valid formats, see
               https://rdflib.readthedocs.io/en/stable/intro_to_parsing.html
               A sample list of valid format values: "turtle", "xml", "n3",
              "nt", "json-ld", "nquads".

            * `base_uri`: Base URI that is prepended to the instance UUID or
              URI (if it is not already a valid URI).

            * `base_prefix`: Optional namespace prefix to use for `base_uri`.

            * `include_meta`: Whether to also serialise metadata. The default
              is to only include metadata if `inst` is a data object.
        """
        self.options = Options(options, defaults="mode=a")
        self.writable = "r" not in self.options.mode
        self.location = location
        self.format = (
            self.options.format
            if "format" in self.options
            else guess_format(location)
        )
        self.graph = rdflib.Graph()
        if self.options.mode in "ra":
            self.graph.parse(location, format=self.format, publicID=PUBLIC_ID)

    def close(self) -> None:
        """Closes this storage."""
        if self.writable:
            self.graph.serialize(self.location, format=self.format)

    def load(self, id: str) -> dlite.Instance:
        """Loads `uuid` from current storage and returns it as a new instance.

        Parameters:
            id: A UUID representing a DLite Instance to return from the RDF
                storage.

        Returns:
            A DLite Instance corresponding to the given `id` (UUID).
        """
        return from_graph(self.graph, id)

    def save(self, inst: dlite.Instance) -> None:
        """Stores `inst` in current storage.

        Parameters:
            inst: A DLite Instance to store in the RDF storage.
        """
        to_graph(
            inst,
            self.graph,
            base_uri=self.options.get("base_uri"),
            base_prefix=self.options.get("base_prefix"),
            include_meta=(
                dlite.asbool(self.options)
                if "include_meta" in self.options
                else None
            ),
        )

    def queue(self, pattern: "Optional[str]" = None) -> "Generator[str]":
        """Generator method that iterates over all UUIDs in the storage
        who"s metadata URI matches glob pattern `pattern`.

        Parameters:
            pattern: A regular expression to filter the yielded UUIDs.

        Yields:
            DLite Instance UUIDs based on the `pattern` regular expression.
            If no `pattern` is given, all UUIDs are yielded from within the
            RDF storage.
        """
        for s, _, o in self.graph.triples((None, DM.hasUUID, None)):
            if pattern:
                metaid = str(list(self.graph.objects(s, DM.instanceOf))[0])
                if dlite.globmatch(pattern, metaid):
                    continue
            yield str(o)

    @classmethod
    def from_bytes(cls, buffer, id=None):
        """Load instance with given `id` from `buffer`.

        Arguments:
            buffer: Bytes or bytearray object to load the instance from.
            id: ID of instance to load.  May be omitted if `buffer` only
                holds one instance.

        Returns:
            New instance.
        """
        return from_rdf(data=buffer, id=id, format=self.options.get("format"))

    @classmethod
    def to_bytes(cls, inst):
        """Save instance `inst` to bytes (or bytearray) object.

        Arguments:
            inst: Instance to save.

        Returns:
            The bytes (or bytearray) object with serialised RDF.
        """
        return to_rdf(
            inst,
            format=self.options.get("format"),
            base_uri=self.options.get("base_uri"),
            base_prefix=self.options.get("base_prefix"),
        )
