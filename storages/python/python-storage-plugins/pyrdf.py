"""A simple demonstrage of a DLite storage plugin written in Python."""
import os
import sys

import rdflib
from rdflib.util import guess_format

import dlite
from dlite.options import Options
from dlite.rdf import DM, PUBLIC_ID, from_graph, to_graph


class pyrdf(dlite.DLiteStorageBase):
    """DLite storage plugin for RDF serialisation."""

    def open(self, uri, options=None):
        """Opens `uri`.

        Supported options:
        - mode : "a" | "r" | "w"
            Valid values are:
            - a   Append to existing file or create new file (default)
            - r   Open existing file for read-only
            - w   Truncate existing file or create new file
        - format : "turtle" | "xml" | "n3" | "nt" | "json-ld" | "nquads"...
            File format.  For a complete list of valid formats, see
            https://rdflib.readthedocs.io/en/stable/intro_to_parsing.html
        - base_uri : str
            Base URI that is prepended to the instance UUID or URI
            (if it is not already a valid URI).
        - base_prefix: str
            Optional namespace prefix to use for `base_uri`.
        - include_meta: bool
            Whether to also serialise metadata.  The default
            is to only include metadata if `inst` is a data object.
        """
        self.options = Options(options, defaults='mode=a')
        self.writable = False if 'r' in self.options.mode else True
        self.uri = uri
        self.format = (
            self.options.format if 'format' in self.options else guess_format(
                uri)
        )
        self.graph = rdflib.Graph()
        if self.options.mode in 'ra':
            self.graph.parse(uri, format=self.format, publicID=PUBLIC_ID)

    def close(self):
        """Closes this storage."""
        if self.writable:
            self.graph.serialize(self.uri, format=self.format)

    def load(self, id):
        """Loads `uuid` from current storage and return it as a new instance."""
        return from_graph(self.graph, id)

    def save(self, inst):
        """Stores `inst` in current storage."""
        to_graph(
            inst,
            self.graph,
            base_uri=self.options.get('base_uri'),
            base_prefix=self.options.get('base_prefix'),
            include_meta=(
                dlite.asbool(self.options) if 'include_meta' in self.options
                else None
            ),
        )

    def queue(self, pattern=None):
        """Generator method that iterates over all UUIDs in the storage
        who's metadata URI matches glob pattern `pattern`."""
        for s, p, o in self.graph.triples((None, DM.hasUUID, None)):
            metaid = str(list(self.graph.objects(s, DM.instanceOf))[0])
            if pattern and dlite.globmatch(pattern, metaid):
                continue
            yield str(o)
