'''A module encapsulating different triplestores using the strategy design
pattern.

See https://github.com/SINTEF/dlite/tree/master/bindings/python/triplestore
for an introduction.

This module has no dependencies outside the standard library, but the
triplestore backends may have.
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
    from typing import Callable, Generator, Tuple, Union

    Triple = Tuple[Union[str, None], Union[str, None], Union[str, None]]


# Regular expression matching a prefixed IRI
_MATCH_PREFIXED_IRI = re.compile(r"^([a-z]+):([^/]{2}.*)$")


class TriplestoreError(Exception):
    """Base exception for triplestore errors."""

class UniquenessError(TriplestoreError):
    """More than one matching triple."""

class NamespaceError(TriplestoreError):
    """Namespace error."""

class NoSuchIRIError(NamespaceError):
    """Namespace has no such IRI."""


class Namespace:
    """Represent a namespace.

    Arguments:
        iri: IRI of namespace to represent.
        label_annotations: Sequence of label annotations. If given, check
            the underlying ontology during attribute access if the name
            correspond to a label. The label annotations should be ordered
            from highest to lowest precedense.
            If True is provided, `label_annotations` is set to
            ``(SKOS.prefLabel, RDF.label, SKOS.altLabel)``.
        check: Whether to check underlying ontology if the IRI exists during
            attribute access.  If true, NoSuchIRIError will be raised if the
            IRI does not exist in this namespace.
        cachemode: Should be one of:
              - Namespace.NO_CACHE: Turn off caching.
              - Namespace.USE_CACHE: Cache attributes as they are looked up.
              - Namespace.ONLY_CACHE: Cache all names at initialisation time.
                Do not access the triplestore after that.
            Default is `NO_CACHE` if neither `label_annotations` or `check`
            is given, otherwise `USE_CACHE`.
        triplestore: Use this triplestore for label lookup and checking.
            If not given, and either `label_annotations` or `check` are
            enabled, a new rdflib triplestore will be created.
        triplestore_url: Alternative URL to use for loading the underlying
            ontology if `triplestore` is not given.  Defaults to `iri`.
    """
    NO_CACHE = 0
    USE_CACHE = 1
    ONLY_CACHE = 2

    __slots__ = (
        "_iri", "_label_annotations", "_check", "_cache", "_triplestore",
    )

    def __init__(self, iri, label_annotations=(), check=False, cachemode=-1,
                 triplestore=None, triplestore_url=None):
        if label_annotations is True:
            label_annotations = (SKOS.prefLabel, RDF.label, SKOS.altLabel)

        self._iri = str(iri)
        self._label_annotations = tuple(label_annotations)
        self._check = bool(check)

        need_triplestore = True if check or label_annotations else False
        if cachemode == -1:
            cachemode = (
                Namespace.ONLY_CACHE if need_triplestore else Namespace.NO_CACHE
            )

        if need_triplestore and triplestore is None:
            url = triplestore_url if triplestore_url else iri
            triplestore = Triplestore("rdflib", base_iri=iri)
            triplestore.parse(url)

        self._cache = {} if cachemode != Namespace.NO_CACHE else None
        #
        # FIXME:
        # Change this to only assigning the triplestore if cachemode is
        # ONLY_CACHE when we figure out a good way to pre-populate the
        # cache with IRIs from the triplestore.
        #
        #self._triplestore = (
        #    triplestore if cachemode != Namespace.ONLY_CACHE else None
        #)
        self._triplestore = triplestore if need_triplestore else None

        if cachemode != Namespace.NO_CACHE:
            self._update_cache(triplestore)

    def _update_cache(self, triplestore=None):
        """Update the internal cache from `triplestore`."""
        if not triplestore:
            triplestore = self._triplestore
        if not triplestore:
            raise NamespaceError(
                "`triplestore` argument needed for updating the cache"
            )
        if self._cache is None:
            self._cache = {}

        # Add (label, full_iri) pairs to cache
        for la in reversed(self._label_annotations):
            self._cache.update(
                (o, s) for s, o in triplestore.subject_objects(la)
                if s.startswith(self._iri)
            )

        # Add (name, full_iri) pairs to cache
        # Currently we only check concepts that defines RDFS.isDefinedBy
        # relations.
        # Is there an efficient way to loop over all IRIs in this namespace?
        n = len(self._iri)
        self._cache.update(
            (s[n:], s) for s in triplestore.subjects(
                RDFS.isDefinedBy, self._iri)
            if s.startswith(self._iri)
        )

    def __getattr__(self, name):
        if self._cache is not None and name in self._cache:
            return self._cache[name]

        if self._triplestore:

            # Check if ``iri = self._iri + name`` is in the triplestore.
            # If so, add it to the cache.
            # We only need to check that generator returned by
            # `self._triplestore.predicate_objects(iri)` is non-empty.
            iri = self._iri + name
            g = self._triplestore.predicate_objects(iri)
            try:
                g.__next__()
            except StopIteration:
                pass
            else:
                if self._cache is not None:
                    self._cache[name] = iri
                return iri

            # Check for label annotations matching `name`.
            for la in self._label_annotations:
                for s, o in self._triplestore.subject_objects(la):
                    if o == name and s.startswith(self._iri):
                        if self._cache is not None:
                            self._cache[name] = s
                        return s

        if self._check:
            raise NoSuchIRIError(self._iri + name)
        else:
            return self._iri + name

    def __getitem__(self, key):
        return self.__getattr__(key)

    def __repr__(self):
        return f"Namespace({self._iri})"

    def __str__(self):
        return self._iri

    def __add__(self, other):
        return self._iri + str(other)


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
PROV = Namespace("http://www.w3.org/ns/prov#")
DCAT = Namespace("http://www.w3.org/ns/dcat#")
TIME = Namespace("http://www.w3.org/2006/time#")
FNO = Namespace("https://w3id.org/function/ontology#")
QUDTU = Namespace("http://qudt.org/vocab/unit/")
OM = Namespace("http://www.ontology-of-units-of-measure.org/resource/om-2/")

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
            warnings.warn(
                f"unknown datatype: {self.datatype} - assuming string"
            )
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

    def __init__(self, backend: str, base_iri: str = None, **kwargs):
        """Initialise triplestore using the backend with the given name.

        Parameters:
            backend: Name of the backend module.
            base_iri: Base IRI used by the add_function() method when adding
                new triples.
            kwargs: Keyword arguments passed to the backend's __init__()
                method.
        """
        module = import_module(backend if "." in backend
                      else "dlite.triplestore.backends." + backend)
        cls = getattr(module, backend.title() + "Strategy")
        self.base_iri = base_iri
        self.namespaces = {}
        self.backend_name = backend
        self.backend = cls(base_iri=base_iri, **kwargs)
        # Keep functions in the triplestore for convienence even though
        # they usually do not belong to the triplestore per se.
        self.function_repo = {}
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
                The rdflib and ontopy backends support e.g. `location`
                (absolute or relative URL) and `data` (string
                containing the data to be parsed) arguments.
        """
        self._check_method("parse")
        self.backend.parse(source=source, format=format, **kwargs)

        if hasattr(self.backend, "namespaces"):
            for prefix, ns in self.backend.namespaces().items():
                if prefix and prefix not in self.namespaces:
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
        """SPARQL query.

        Parameters:
            query_object: String with the SPARQL query.
            kwargs: Keyword arguments passed to rdflib.Graph.query().

        Returns:
            List of tuples of IRIs for each matching row.

        Note:
            This method is intended for SELECT queries.  Use
            the update() method for INSERT and DELETE  queries.

        """
        self._check_method("query")
        return self.backend.query(query_object=query_object, **kwargs)

    def update(self, update_object, **kwargs):
        """Update triplestore with SPARQL.

        Parameters:
            query_object: String with the SPARQL query.
            kwargs: Keyword arguments passed to rdflib.Graph.query().

        Note:
            This method is intended for INSERT and DELETE queries.  Use
            the query() method for SELECT queries.
        """
        self._check_method("update")
        return self.backend.update(update_object=update_object, **kwargs)

    def bind(self, prefix: str, namespace: "Union[str, Namespace]", **kwargs):
        """Bind prefix to namespace and return the new Namespace object.

        The new Namespace is created with `namespace` as IRI.
        Keyword arguments are passed to the Namespace() constructor.

        If `namespace` is None, the corresponding prefix is removed.
        """
        if namespace is None:
            del self.namespaces[prefix]
            ns = None
        else:
            ns = namespace if isinstance(namespace, Namespace) else Namespace(
                namespace, **kwargs)
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
                f'Triplestore backend "{self.backend_name}" do not '
                f'implement a "{name}()" method.')

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

    def has(self, subject=None, predicate=None, object=None):
        """Returns true if the triplestore has any triple matching
        the give subject, predicate and/or object."""
        g = self.triples((subject, predicate, object))
        try:
            g.__next__()
        except StopIteration:
            return False
        return True


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

        This is the reverse of expand_iri().

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

    def add_mapsTo(
            self,
            target: str,
            source: "Union[str, dlite.Instance, dataclass]",
            property_name: str = None,
            cost: "Union[float, Callable]" = None,
            target_cost: bool = True,
    ):
        """Add 'mapsTo' relation to triplestore.

        Parameters:
            target: IRI of target ontological concept.
            source: Source IRI or entity object.
            property_name: Name of property if `source` is an entity or
                an entity IRI.
            cost: User-defined cost of following this mapping relation
                represented as a float.  It may be given either as a
                float or as a callable taking the value of the mapped
                quantity as input and returning the cost as a float.
            target_cost: Whether the cost is assigned to mapping steps
                that have `target` as output.
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
        if cost is not None:
            dest = target if target_cost else source
            self._add_cost(cost, dest)

    def add_function(
            self,
            func: Callable,
            expects: "Union[str, Sequence, Mapping]" = (),
            returns: "Union[str, Sequence]" = (),
            base_iri: str = None,
            standard: str = 'fno',
            cost: "Union[float, Callable]" = None,
    ):
        """Inspect function and add triples describing it to the triplestore.

        Parameters:
            func: Function to describe.
            expects: Sequence of IRIs to ontological concepts corresponding
                to positional arguments of `func`.  May also be given as a
                dict mapping argument names to corresponding ontological IRIs.
            returns: IRI of return value.  May also be given as a sequence
                of IRIs, if multiple values are returned.
            base_iri: Base of the IRI representing the function in the
                knowledge base.  Defaults to the base IRI of the triplestore.
            standard: Name of ontology to use when describing the function.
                Defaults to the Function Ontology (FnO).
            cost: User-defined cost of following this mapping relation
                represented as a float.  It may be given either as a
                float or as a callable taking the same arguments as `func`
                returning the cost as a float.

        Returns:
            func_iri: IRI of the added function.
        """
        if isinstance(expects, str):
            expects = [expects]
        if isinstance(returns, str):
            returns = [returns]

        method = getattr(self, f"_add_function_{standard}")
        func_iri = method(func, expects, returns, base_iri)
        self.function_repo[func_iri] = func

        if cost is not None:
            for dest_iri in returns:
                self._add_cost(cost, dest_iri)

        return func_iri

    def _add_cost(self, cost, dest_iri):
        """Help function that adds `cost` to destination IRI `dest_iri`.

        `cost` should be either a float or a Callable returning a float.

        If `cost` is a callable it is just referred to with a literal
        id and is not ontologically described as a function.  The
        expected input arguments depends on the context, which is why
        this function is not part of the public API.  Use the add_mapsTo()
        and add_function() methods instead.
        """
        if self.has(dest_iri, DM.hasCost):
            warnings.warn(f"A cost is already assigned to IRI: {dest_iri}")
        elif callable(cost):
            cost_id = f"cost_function{function_id(cost)}"
            self.add((dest_iri, DM.hasCost, Literal(cost_id)))
            self.function_repo[cost_id] = cost
        else:
            self.add((dest_iri, DM.hasCost, Literal(cost)))

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

        lst = outlist
        for i, iri in enumerate(returns):
            lst_next = f"{outlist}{i+2}" if i < len(returns) - 1 else RDF.nil
            val = f"{func_iri}_output{i+1}"
            self.add((val, RDF.type, FNO.Output))
            self.add((val, MAP.mapsTo, iri))
            self.add((lst, RDF.first, val))
            self.add((lst, RDF.rest, lst_next))
            lst = lst_next

        return func_iri


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
