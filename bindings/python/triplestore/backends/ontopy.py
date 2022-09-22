import io
import os
import tempfile
import warnings
from typing import TYPE_CHECKING

from ontopy.ontology import get_ontology, Ontology, _unabbreviate


from dlite.triplestore import Literal

if TYPE_CHECKING:  # pragma: no cover
    from collections.abc import Sequence
    from typing import Generator

    from dlite.triplestore import Triple


class OntopyStrategy:
    """Triplestore strategy for EMMOntoPy.

    Arguments:
        base_iri: The base iri of the ontology.
        onto: Ontology to initiate the triplestore from.  Defaults to an new
            ontology with the given `base_iri`.
        load: Whether to load the ontology.
        kwargs: Keyword arguments passed to the ontology load() method.

    Either the `base_iri` or `onto` argument must be provided.
    """
    def __init__(
            self,
            base_iri: str = None,
            onto: Ontology = None,
            load: bool = False,
            kwargs: dict = {}
    ):
        if onto is None:
            if base_iri is None:
                raise TypeError("either `base_iri` or `onto` must be provided")
            self.onto = get_ontology(base_iri)
        elif isinstance(onto, Ontology):
            self.onto = onto
        else:
            raise TypeError("`onto` must be either an ontology or None")

        if load:
            self.onto.load(**kwargs)

    def triples(self, triple: "Triple") -> "Generator":
        """Returns a generator over matching triples."""

        def to_literal(o, d):
            """Returns a literal from (o, d)."""
            if isinstance(d, str) and d.startswith("@"):
                lang, datatype = d[1:], None
            else:
                lang, datatype = None, d
            return Literal(o, lang=lang, datatype=datatype)

        s, p, o = triple
        abb = (
            None if s is None else self.onto._abbreviate(s),
            None if p is None else self.onto._abbreviate(p),
            None if o is None else self.onto._abbreviate(o),
        )
        for s, p, o in self.onto._get_obj_triples_spo_spo(*abb):
            yield (
                _unabbreviate(self.onto, s),
                _unabbreviate(self.onto, p),
                _unabbreviate(self.onto, o),
            )
        for s, p, o, d in self.onto._get_data_triples_spod_spod(*abb, d=''):
            yield (
                _unabbreviate(self.onto, s),
                _unabbreviate(self.onto, p),
                to_literal(o, d),
            )

    def add_triples(self, triples: "Sequence[Triple]"):
        """Add a sequence of triples."""
        for s, p, o in triples:
            if isinstance(o, Literal):
                if o.lang:
                    d = f"@{o.lang}"
                elif o.datatype:
                    d = f"^^{o.datatype}"
                else:
                    d = 0
                self.onto._add_data_triple_spod(
                    self.onto._abbreviate(s),
                    self.onto._abbreviate(p),
                    self.onto._abbreviate(o),
                    d,
                )
            else:
                self.onto._add_obj_triple_spo(
                    self.onto._abbreviate(s),
                    self.onto._abbreviate(p),
                    self.onto._abbreviate(o),
                )

    def remove(self, triple: "Triple"):
        """Remove all matching triples from the backend."""
        s, p, o = triple
        to_remove = list(self.onto._get_triples_spod_spod(
            self.onto._abbreviate(s) if s is not None else None,
            self.onto._abbreviate(p) if s is not None else None,
            self.onto._abbreviate(o) if s is not None else None,
        ))
        for s, p, o, d in to_remove:
            if d:
                self.onto._del_data_triple_spod(s, p, o, d)
            else:
                self.onto._del_obj_triple_spo(s, p, o)

    # Optional methods
    def parse(self, source=None, location=None, data=None, format=None,
              encoding=None, **kwargs):
        """Parse source and add the resulting triples to triplestore.

        The source is specified using one of `source`, `location` or `data`.

        Parameters:
            source: File-like object or file name.
            location: String with relative or absolute URL to source.
            data: String containing the data to be parsed.
            format: Needed if format can not be inferred from source.
            encoding: Encoding argument to io.open().
            kwargs: Additional keyword arguments passed to Ontology.load().
        """
        if sum(arg is not None for arg in (source, location, data)) != 1:
            raise ValueError(
                "one (and only one) of `source`, `location` and `data` "
                "should be provided")

        if source:
            self.onto.load(filename=source, format=format, **kwargs)
        elif location:
            self.onto.load(filename=location, format=format, **kwargs)
        elif data:
            #s = io.StringIO(data)
            #self.onto.load(filename=s, format=format, **kwargs)

            # Could have been done much nicer if it hasn't been for Windows
            filename = None
            try:
                kw = {"delete": False}
                if isinstance(data, str):
                    kw.update(mode="w+t", encoding=encoding)
                with tempfile.NamedTemporaryFile(**kw) as f:
                    f.write(data)
                    filename = f.name
                self.onto.load(filename=filename, format=format, **kwargs)
            finally:
                if filename:
                    os.remove(filename)

        else:
            raise ValueError(
                "either `source`, `location` or `data` must be given"
            )

    def serialize(self, destination=None, format='turtle', **kwargs):
        """Serialise to destination.

        Parameters:
            destination: File name or object to write to.  If None, the
                serialisation is returned.
            format: Format to serialise as.  Supported formats, depends on
                the backend.
            kwargs: Passed to the Ontology.save() method.

        Returns:
            Serialised string if `destination` is None.
        """
        if destination:
            self.onto.save(destination, format=format, **kwargs)
        else:
            # Clumsy implementation due to Windows file locking...
            filename = None
            try:
                with tempfile.NamedTemporaryFile(delete=False) as f:
                    filename = f.name
                    self.onto.save(filename, format=format, **kwargs)
                with open(filename, 'rt') as f:
                    return f.read()
            finally:
                if filename:
                    os.remove(filename)

    def query(self, query_object, native=True, **kwargs):
        """SPARQL query."""
        if native:
            res = self.onto.world.sparql(query_object)
        else:
            graph = self.onto.world.as_rdflib_graph()
            res = graph.query(query_object, **kwargs)
        # TODO: Convert result to expected type
        return res

    def update(self, update_object, native=True, **kwargs):
        """Update triplestore with SPARQL."""
        if native:
            self.onto.world.sparql(update_object)
        else:
            graph = self.onto.world.as_rdflib_graph()
            graph.update(update_object, **kwargs)
