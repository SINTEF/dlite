'''A module encapsulating different triplestores using the strategy design
pattern.

This module has no dependencies outside the standard library, but the
triplestore backends have.

The main class is Triplestore, who's __init__() method takes the name of the
backend to encapsulate as first argument.  It's interface is strongly inspired
by rdflib.Graph, but simplified when possible to make it easy to use.  Some
important differences:
- all IRIs are represented by Python strings
- blank nodes are strings starting with "_:"
- literals are constructed with Literal()

The module already provides a set of pre-defined namespaces that simplifies
writing IRIs. For example:

    >>> from triplestore import RDFS, OWL
    >>> RDFS.subClassOf
    'http://www.w3.org/2000/01/rdf-schema#subClassOf'

New namespaces can be created using the Namespace class, but are usually
added with the bind() method:

    >>> from triplestore import Triplestore
    >>> ts = Triplestore(backend="rdflib")
    >>> ONTO = ts.bind("onto", "http://example.com/onto#")
    >>> ONTO.MyConcept
    'http://example.com/onto#MyConcept'

New triples can added either with the parse() method (for backends that support
it) or the add() and add_triples() methods.

    # en(msg) is a convinient function for adding english literals.
    # It is equivalent to ``triplestore.Literal(msg, lang="en")``.
    >>> from triplestore import en
    >>> ts.parse("onto.ttl", format="turtle")
    >>> ts.add_triples([
    ...     (ONTO.MyConcept, RDFS.subClassOf, OWL.Thing),
    ...     (ONTO.MyConcept, RDFS.label, en("My briliant ontological concept.")),
    ... ])

For backends that support it can the triplestore be serialised using
serialize():

    >>> ts.serialize("onto2.ttl")

A set of convenient functions exists for simple queries, including
triples(), subjects(), predicates(), objects(), subject_predicates(),
subject_objects(), predicate_objects() and value().  Except for value(),
they return the result as generators. For example:

    >>> list(ts.objects(subject=ONTO.MyConcept, predicate=RDFS.subClassOf))
    ['http://www.w3.org/2002/07/owl#Thing']

The query() and update() methods can be used to query and update the
triplestore using SPARQL.

Finally Triplestore has two specialised methods add_mapsTo() and
add_function() that simplify working with mappings.  add_mapsTo() is
convinient for defining new mappings:

    >>> from triplestore import Namespace
    >>> META = Namespace("http://onto-ns.com/meta/0.1/MyEntity#")
    >>> ts.add_mapsTo(ONTO.MyConcept, META.my_property)

It can also be used with DLite and SOFT7 data models.  Here we repeat
the above with DLite:

    >>> import dlite
    >>> meta = dlite.get_entity("http://onto-ns.com/meta/0.1/MyEntity")
    >>> ts.add_mapsTo(ONTO.MyConcept, meta, "my_property")

The add_function() describes a function and adds mappings for its
arguments and return value(s).  Currently it only supports the Function
Ontology (FnO).

    >>> def mean(x, y):
    ...     """Returns the mean value of `x` and `y`."""
    ...     return (x + y)/2

    >>> ts.add_function(mean,
    ...                 expects=(ONTO.RightArmLength, ONTO.LeftArmLength),
    ...                 returns=ONTO.AverageArmLength)


TODO:
* Update the query() method to return the SPARQL result in a backend-
  independent way.
* Add additional backends. Candidates include:
    - list of tuples
    - owlready2/EMMOntoPy
    - Stardog
    - DLite triplestore (based on Redland librdf)
    - Redland librdf
    - Apache Jena Fuseki
    - Allegrograph

'''
from __future__ import annotations  # Support Python 3.7 (PEP 585)

import hashlib
import inspect
import re
import warnings
from collections.abc import Sequence
from datetime import datetime
from importlib import import_module
from typing import TYPE_CHECKING

if TYPE_CHECKING:  # pragma: no cover
    from collections.abc import Mapping
    from typing import Generator, Tuple, Union

    Triple = Tuple[Union[str, None], Union[str, None], Union[str, None]]


# Regular expression matching a prefixed IRI
_MATCH_PREFIXED_IRI = re.compile(r"^([a-z]+):([^/]{2}.*)$")


class TriplestoreError(Exception):
    """Base exception for triplestore errors."""

class UniquenessError(TriplestoreError):
    """More than one matching triple."""

class NamespaceError(TriplestoreError):
    """Namespace error."""


class Namespace:
    """Represent a namespace."""
    def __init__(self, uri):
        self.uri = str(uri)

    def __getattr__(self, name):
        return self.uri + name

    def __getitem__(self, key):
        return self.uri + key

    def __repr__(self):
        return f"Namespace({self.iri})"

    def __str__(self):
        return self.uri


# Pre-defined namespaces
XML = Namespace("http://www.w3.org/XML/1998/namespace")
RDF = Namespace("http://www.w3.org/1999/02/22-rdf-syntax-ns#")
RDFS = Namespace("http://www.w3.org/2000/01/rdf-schema#")
XSD = Namespace("http://www.w3.org/2001/XMLSchema#")
OWL = Namespace("http://www.w3.org/2002/07/owl#")
SKOS = Namespace("http://www.w3.org/2004/02/skos/core#")
DC = Namespace("http://purl.org/dc/elements/1.1/")
DCTERMS = Namespace("http://purl.org/dc/terms/")
FOAF = Namespace("http://xmlns.com/foaf/0.1/")
DOAP = Namespace("http://usefulinc.com/ns/doap#")
FNO = Namespace("https://w3id.org/function/ontology#")

EMMO = Namespace("http://emmo.info/emmo#")
MAP = Namespace("http://emmo.info/domain-mappings#")
DM = Namespace("http://emmo.info/datamodel#")


class Literal(str):
    """A literal RDF value."""
    def __new__(cls, value, lang=None, datatype=None):
        string = super().__new__(cls, value)
        if lang:
            if datatype:
                raise TypeError("A literal can only have one of `lang` or "
                                "`datatype`.")
            string.lang = str(lang)
            string.datatype = None
        else:
            string.lang = None
            if datatype:
                d = {
                    str: XSD.string,
                    bool: XSD.boolean,
                    int: XSD.integer,
                    float: XSD.double,
                    bytes: XSD.hexBinary,
                    bytearray: XSD.hexBinary,
                    datetime: XSD.dateTime,
                }
                string.datatype = d.get(datatype, datatype)
            elif isinstance(value, str):
                string.datatype = None
            elif isinstance(value, bool):
                string.datatype = XSD.boolean
            elif isinstance(value, int):
                string.datatype = XSD.integer
            elif isinstance(value, float):
                string.datatype = XSD.double
            elif isinstance(value, (bytes, bytearray)):
                string = value.hex()
                string.datatype = XSD.hexBinary
            elif isinstance(value, datetime):
                string.datatype = XSD.dateTime
                # TODO:
                #   - XSD.base64Binary
                #   - XSD.byte, XSD.unsignedByte
            else:
                string.datatype = None
        return string

    def __repr__(self):
        lang = f", lang='{self.lang}'" if self.lang else ""
        datatype = f", datatype='{self.datatype}'" if self.datatype else ""
        return f"Literal('{self}'{lang}{datatype})"

    value = property(
        fget=lambda self: self.to_python(),
        doc="Appropriate python datatype derived from this RDF literal.",
    )

    def to_python(self):
        """Returns an appropriate python datatype derived from this RDF
        literal."""
        v = str(self)

        if self.datatype == XSD.boolean:
            v = bool(self)
        elif self.datatype in (XSD.integer, XSD.int, XSD.short, XSD.long,
                               XSD.nonPositiveInteger,
                               XSD.negativeInteger, XSD.nonNegativeInteger,
                               XSD.unsignedInt, XSD.unsignedShort,
                               XSD.unsignedLong,
                               XSD.byte, XSD.unsignedByte,
                               ):
            v = int(self)
        elif self.datatype in (XSD.double, XSD.decimal, XSD.dataTimeStamp,
                               OWL.real, OWL.rational):
            v = float(self)
        elif self.datatype == XSD.hexBinary:
            v = self.encode()
        elif self.datatype == XSD.dateTime:
            v = datetime.fromisoformat(self)
        elif self.datatype and self.datatype not in (
                RDF.PlainLiteral, RDF.XMLLiteral, RDFS.Literal,
                XSD.anyURI, XSD.language, XSD.Name, XSD.NMName,
                XSD.normalizedString, XSD.string, XSD.token, XSD.NMTOKEN,
        ):
            warnings.warn(f"unknown datatype: {self.datatype} - assuming string")
        return v

    def n3(self):
        """Returns a representation in n3 format."""
        if self.lang:
            return f'"{self}"@{self.lang}'
        elif self.datatype:
            return f'"{self}"^^{self.datatype}'
        else:
            return f'"{self}"'


def en(value):
    """Convenience function that returns value as a plain english literal.

    Equivalent to``Literal(value, lang="en")``.
    """
    return Literal(value, lang="en")


class Triplestore:
    """Provides a common frontend to a range of triplestore backends."""

    default_namespaces = {
        "xml": XML,
        "rdf": RDF,
        "rdfs": RDFS,
        "xsd": XSD,
        "owl": OWL,
        # "skos": SKOS,
        # "dc": DC,
        # "dcterms": DCTERMS,
        # "foaf": FOAF,
        # "doap": DOAP,
        # "fno": FNO,
        # "emmo": EMMO,
        # "map": MAP,
        # "dm": DM,
    }

    def __init__(self, name: str, base_iri: str = None, **kwargs):
        """Initialise triplestore using the backend with the given name.

        Parameters:
            name: Module name for backend.
            base_iri: Base IRI used by the add_function() method when adding
                new triples.
            kwargs: Keyword arguments passed to the backend's __init__() method.
        """
        module = import_module(name if "." in name
                      else "dlite.triplestore.backends." + name)
        cls = getattr(module, name.title() + "Strategy")
        self.base_iri = base_iri
        self.namespaces = {}
        self.backend_name = name
        self.backend = cls(**kwargs)
        for prefix, ns in self.default_namespaces.items():
            self.bind(prefix, ns)

    # Methods implemented by backend
    # ------------------------------
    def triples(self, triple: "Triple") -> "Generator":
        """Returns a generator over matching triples."""
        return self.backend.triples(triple)

    def add_triples(self, triples: "Sequence[Triple]"):
        """Add a sequence of triples."""
        self.backend.add_triples(triples)

    def remove(self, triple: "Triple"):
        """Remove all matching triples from the backend."""
        self.backend.remove(triple)

    # Methods optionally implemented by backend
    # -----------------------------------------
    def parse(self, source=None, format=None, **kwargs):
        """Parse source and add the resulting triples to triplestore.

        Parameters:
            source: File-like object or file name.
            format: Needed if format can not be inferred from source.
            kwargs: Keyword arguments passed to the backend.
                The rdflib backend supports e.g. `location` (absolute
                or relative URL) and `data` (string containing the
                data to be parsed) arguments.
        """
        self._check_method("parse")
        self.backend.parse(source=source, format=format, **kwargs)

        if hasattr(self.backend, "namespaces"):
            for prefix, ns in self.backend.namespaces().items():
                if prefix not in self.namespaces:
                    self.namespaces[prefix] = Namespace(ns)

    def serialize(self, destination=None, format="turtle", **kwargs):
        """Serialise triplestore.

        Parameters:
            destination: File name or object to write to.  If None, the
                serialisation is returned.
            format: Format to serialise as.  Supported formats, depends on
                the backend.
            kwargs: Passed to the backend serialize() method.

        Returns:
            Serialized string if `destination` is None.
        """
        self._check_method("serialize")
        return self.backend.serialize(destination=destination,
                                      format=format, **kwargs)

    def query(self, query_object, **kwargs):
        """SPARQL query."""
        self._check_method("query")
        return self.backend.query(query_object=query_object, **kwargs)

    def update(self, update_object, **kwargs):
        """Update triplestore with SPARQL."""
        self._check_method("update")
        return self.backend.update(update_object=update_object, **kwargs)

    def bind(self, prefix: str, namespace: str):
        """Bind prefix to namespace and return the new Namespace object.

        If `namespace` is None, the corresponding prefix is removed.
        """
        if namespace is None:
            del self.namespaces[prefix]
            ns = None
        else:
            ns = namespace if isinstance(namespace, Namespace) else Namespace(
                namespace)
            self.namespaces[prefix] = ns

        if hasattr(self.backend, "bind"):
            self.backend.bind(prefix, namespace)

        return ns

    # Convenient methods
    # ------------------
    # These methods are modelled after rdflib and provide some convinient
    # interfaces to the triples(), add_triples() and remove() methods
    # implemented by all backends.
    def _check_method(self, name):
        """Check that backend implements the given method."""
        if not hasattr(self.backend, name):
            raise NotImplementedError(
                f"Triplestore backend \"{self.backend_name}\" doesn't implement "
                f"a \"{name}()\" method.")

    def add(self, triple: "Triple"):
        """Add `triple` to triplestore."""
        self.add_triples([triple])

    def value(self, subject=None, predicate=None, object=None, default=None,
              any=False):
        """Return the value for a pair of two criteria.

        Useful if one knows that there may only be one value.

        Parameters:
            subject, predicate, object: Triple to match.
            default: Value to return if no matches are found.
            any: If true, return any matching value, otherwise raise
                UniquenessError.
        """
        g = self.triples((subject, predicate, object))
        try:
            value = next(g)
        except StopIteration:
            return default

        if any:
            return value

        try:
            next(g)
        except StopIteration:
            return value
        else:
            raise UniquenessError("More than one match")

    def subjects(self, predicate=None, object=None):
        """Returns a generator of subjects for given predicate and object."""
        for s, _, _ in self.triples((None, predicate, object)):
            yield s

    def predicates(self, subject=None, object=None):
        """Returns a generator of predicates for given subject and object."""
        for _, p, _ in self.triples((subject, None, object)):
            yield p

    def objects(self, subject=None, predicate=None):
        """Returns a generator of objects for given subject and predicate."""
        for _, _, o in self.triples((subject, predicate, None)):
            yield o

    def subject_predicates(self, object=None):
        """Returns a generator of (subject, predicate) tuples for given
        object."""
        for s, p, _ in self.triples((None, None, object)):
            yield s, p

    def subject_objects(self, predicate=None):
        """Returns a generator of (subject, object) tuples for given
        predicate."""
        for s, _, o in self.triples((None, predicate, None)):
            yield s, o

    def predicate_objects(self, subject=None):
        """Returns a generator of (predicate, object) tuples for given
        subject."""
        for _, p, o in self.triples((subject, None, None)):
            yield p, o

    def set(self, triple):
        """Convenience method to update the value of object.

        Removes any existing triples for subject and predicate before adding
        the given `triple`.
        """
        s, p, _ = triple
        self.remove((s, p, None))
        self.add(triple)

    # Methods providing additional functionality
    # ------------------------------------------
    def expand_iri(self, iri: str):
        """Return the full IRI if `iri` is prefixed.  Otherwise `iri` is
        returned."""
        match = re.match(_MATCH_PREFIXED_IRI, iri)
        if match:
            prefix, name = match.groups()
            if prefix not in self.namespaces:
                raise NamespaceError(f"unknown namespace: {prefix}")
            return f"{self.namespaces[prefix]}{name}"
        return iri

    def prefix_iri(self, iri: str, require_prefixed: bool = False):
        """Return prefixed IRI.

        This is the referse of expand_iri().

        If `require_prefixed` is true, a NamespaceError exception is raised
        if no prefix can be found.
        """
        if not re.match(_MATCH_PREFIXED_IRI, iri):
            for prefix, namespace in self.namespaces.items():
                if iri.startswith(str(namespace)):
                    return f"{prefix}:{iri[len(str(namespace)):]}"
            if require_prefixed:
                raise NamespaceError(f"No prefix defined for IRI: {iri}")
        return iri

    def add_mapsTo(self,
                   target: str,
                   source: "Union[str, dlite.Instance, dataclass]",
                   property_name: str = None
                   ):
        """Add 'mapsTo' relation to triplestore.

        Parameters:
            target: IRI of target ontological concept.
            source: Source IRI or entity object.
            property_name: Name of property if `source` is an entity or
                an entity IRI.
        """
        self.bind("map", MAP)

        if not property_name and not isinstance(source, str):
            raise TriplestoreError(
                "`property_name` is required when `target` is not a string.")

        target = self.expand_iri(target)
        source = self.expand_iri(infer_iri(source))
        if property_name:
            source = f"{source}#{property_name}"
            self.add((source, MAP.mapsTo, target))

    def add_function(self,
                     func: callable,
                     expects: "Union[Sequence, Mapping]" = (),
                     returns: "Union[str, Sequence]" = (),
                     base_iri: str = None,
                     standard: str = 'fno',
                     ):
        """Inspect function and add triples describing it to the triplestore.

        Parameters:
            func: Function to describe.
            expects: Sequence of IRIs to ontological concepts corresponding
                to positional arguments of `func`.  May also be given as a
                dict mapping argument names to corresponding ontological IRIs.
            returns: IRI of return value.  May also be given as a sequence
                of IRIs, if multiple values are returned.
            base_iri:
            standard: Name of ontology to use when describing the function.
                Defaults to the Function Ontology (FnO).
        """
        method = getattr(self, f"_add_function_{standard}")
        return method(func, expects, returns, base_iri)

    def _add_function_fno(self, func, expects, returns, base_iri):
        """Implementing add_function() for FnO."""
        self.bind("fno", FNO)
        self.bind("dcterms", DCTERMS)
        self.bind("map", MAP)

        if base_iri is None:
            base_iri = self.base_iri if self.base_iri else ":"
        fid = function_id(func)  # Function id
        doc = inspect.getdoc(func)
        name = func.__name__
        signature = inspect.signature(func)
        func_iri = f"{base_iri}{name}_{fid}"
        parlist = f"_:{name}{fid}parlist"
        outlist = f"_:{name}{fid}outlist"
        self.add((func_iri, RDF.type, FNO.Function))
        self.add((func_iri, FNO.expects, parlist))
        self.add((func_iri, FNO.returns, outlist))
        if doc:
            self.add((func_iri, DCTERMS.description, en(doc)))

        if isinstance(expects, Sequence):
            items = list(zip(expects, signature.parameters))
        else:
            items = [(expects[par], par)
                     for par in signature.parameters.keys()]
        lst = parlist
        for i, (iri, parname) in enumerate(items):
            lst_next = f"{parlist}{i+2}" if i < len(items) - 1 else RDF.nil
            par = f"{func_iri}_parameter{i+1}_{parname}"
            self.add((par, RDF.type, FNO.Parameter))
            self.add((par, RDFS.label, en(parname)))
            self.add((par, MAP.mapsTo, iri))
            self.add((lst, RDF.first, par))
            self.add((lst, RDF.rest, lst_next))
            lst = lst_next

        if isinstance(returns, str):
            returns = [returns]
        lst = outlist
        for i, iri in enumerate(returns):
            lst_next = f"{outlist}{i+2}" if i < len(returns) - 1 else RDF.nil
            val = f"{func_iri}_output{i+1}"
            self.add((val, RDF.type, FNO.Output))
            self.add((val, MAP.mapsTo, iri))
            self.add((lst, RDF.first, val))
            self.add((lst, RDF.rest, lst_next))
            lst = lst_next


def infer_iri(obj):
    """Return IRI of the individual that stands for object `obj`."""
    if isinstance(obj, str):
        return obj
    if hasattr(obj, "uri") and obj.uri:
        # dlite.Metadata or dataclass (or instance with uri)
        return obj.uri
    if hasattr(obj, "uuid") and obj.uuid:
        # dlite.Instance or dataclass
        return obj.uuid
    if hasattr(obj, "schema") and callable(obj.schema):
        # pydantic.BaseModel
        schema = obj.schema()
        properties = schema['properties']
        if "uri" in properties and properties["uri"]:
            return properties["uri"]
        if "uuid" in properties and properties["uuid"]:
            return properties["uuid"]
    raise TypeError("cannot infer IRI from object {obj!r}")


def function_id(func, length=4):
    """Return a checksum for function `func`.

    The returned object is a string of hexadecimal digits.

    `length` is the number of bytes in the returned checksum.  Since
    the current implementation is based on the shake_128 algorithm,
    it make no sense to set `length` larger than 32 bytes.
    """
    #return hex(crc32(inspect.getsource(func).encode())).lstrip('0x')
    return hashlib.shake_128(
        inspect.getsource(func).encode()).hexdigest(length)
