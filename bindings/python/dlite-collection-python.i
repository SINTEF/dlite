/* -*- Python -*-  (not really, but good for syntax highlighting) */


%extend struct _CollectionIter {
%pythoncode %{
    def __iter__(self):
        return self

    def __next__(self):
        if self.rettype == 'I':  # instance
            v = self.next()
            if v and v.is_meta:
                v.__class__ = Metadata
        elif self.rettype in 'RTtspod':  # relation
            v = self.next_relation()
        else:
            raise ValueError('`rettype` must one of "IRTtspod"')

        if v is None:
            raise StopIteration()

        if self.rettype == 'I':  # return instance
            return v
        if self.rettype == 'R':  # return relation object
            return v.copy()
        if self.rettype == 'T':  # return (s,p,o,d) tuple
            return (v.s, v.p, v.o, v.d)
        if self.rettype == 't':  # return (s,p,o) tuple
            return (v.s, v.p, v.o)
        elif self.rettype == 's':  # return subject
            return v.s
        elif self.rettype == 'p':  # return predicate
            return v.p
        elif self.rettype == 'o':  # return object
            return v.o
        elif self.rettype == 'd':  # return datatype
            return v.d

        raise _dlite.DLiteRuntimeError()  # should never be reached
%}

}


%pythoncode %{
from warnings import warn


class Collection(Instance):
    """A collection of instances and relations between them.

    Instances added to a collection are referred to with a label,
    which is local to the collection.

    Iteration over a collection will iterate over the instances added
    to it.  Likewise, the len() of a collection will return the number
    of instances.

    Use the `get_relations()` method (or the convenience methods
    `get_subjects()`, `get_predicates()` and `get_objects()`) to iterate
    over relations.  The number of relations is available via the
    `nrelations` property.

    Relations are `(s, p, o, d=None)`-triples with an optional fourth field
    `d`, specifying the datatype of the object.  The datatype may have the
    following values:

    - None: object is an IRI.
    - Starts with '@': object is a language-tagged plain literal.
      The language identifier follows the '@'-sign. e.g. `@en`
    - Otherwise: object is a literal with datatype `d`.

    Arguments:
        id: URI or UUID of the new instance.  The default is to create a
            collection with no URI and random (version 4) UUID.

    """
    def __new__(cls, id=None):
        """Creates an empty collection."""
        _coll = _Collection(id=id)
        inst = _coll.asinstance()
        inst.__class__ = Collection
        return inst

    def __init__(self, id=None):
        pass  # avoid calling Instance.__init__()

    @classmethod
    def load(cls, src, location=None, options=None, id=None, lazy=True):
        """Loads a collection from storage.

        Arguments:
            src: Storage instance | url | driver
            location:
                File path to load from when `src` is a driver.
            options: str
                Options passed to the storage plugin when `src` is a driver.
            id: str
                Id of collection to load.  Needed if there are more than
                one instance in the storage.
            lazy: bool
                Whether to also save all instances referred to by the
                collection.
        """
        if isinstance(src, Storage):
            _coll = _Collection(storage=src, id=id, lazy=bool(lazy))
        elif location:
            with Storage(src, location, options) as s:
                _coll = _Collection(storage=s, id=id, lazy=bool(lazy))
        else:
            _coll = _Collection(url=src, lazy=bool(lazy))
        inst = _coll.asinstance()
        inst.__class__ = Collection
        return inst

    def save(self, dst, location=None, options=None, include_instances=True):
        """Save collection.

        Arguments:
            dst: Storage instance | url | driver
            location: str
                File path to write to when `dst` is a driver.
            options: str
                Options passed to the storage plugin when `dst` is a driver.
            include_instances: bool
                Whether to also save all instances referred to by the
                collection.
        """
        loc = str(location) if location else None
        if include_instances:
            c = self._coll
            if isinstance(dst, Storage):
                _collection_save(c, dst)
            elif location:
                with Storage(dst, loc, options) as s:
                    _collection_save(c, s)
            else:
                _collection_save_url(c, dst)
        elif isinstance(dst, Storage):
            Instance.save(self, storage=dst)
        else:
            Instance.save(self, dst, loc, options)

    _coll = property(
        lambda self: _get_collection(id=self.uuid),
        doc='SWIG-internal representation of this collection.')

    properties = property(
        lambda self: {'relations': list(self.get_relations())},
        doc='Dictionary with property name-value pairs.')

    def __repr__(self):
        return '<Collection: %s>' % (
            f"uri='{self.uri}'" if self.uri else f"uuid='{self.uuid}'")

    def __iter__(self):
        return _CollectionIter(self, rettype='I')

    def __len__(self):
        return _collection_count(self._coll)

    def __getitem__(self, label):
        return self.get(label)

    def __setitem__(self, label, inst):
        self.add(label, inst)

    def __delitem__(self, label):
        self.remove(label)

    def __contains__(self, label):
        return self.has(label)

    def add(self, label, inst, force=False):
        """Add `inst` to collection with given label.
        If `force` is true, a possible existing instance will be replaced.
        """
        if force and self.has(label):
            self.remove(label)
        _collection_add(self._coll, label, inst)

    def remove(self, label):
        """Remove instance with given label from collection."""
        if _collection_remove(self._coll, label):
            raise _dlite.DLiteError(f'No such label in collection: "{label}"')

    def get(self, label, metaid=None):
        """Return instance with given label.

        If `metaid` is given, the instance is converted to an instance
        of `metaid` using instance mappings.  If no such instance mapping
        is registered, a DLiteError is raised.
        """
        inst = _collection_get_new(self._coll, label, metaid)
        if inst.is_meta:
            inst.__class__ = Metadata
        elif inst.meta.uri == COLLECTION_ENTITY:
            inst.__class__ = Collection
        return inst

    def get_id(self, id):
        """Return instance with given id."""
        inst = _collection_get_id(self._coll, id)
        return inst

    def has(self, label):
        """Returns true if an instance has been added with the given label."""
        b = _collection_has(self._coll, label)
        return bool(b)

    def has_id(self, id):
        """Returns true if an instance has been added with the given id."""
        b = _collection_has_id(self._coll, id)
        return bool(b)

    def add_relation(self, s, p, o, d=None):
        """Add (subject, predicate, object) RDF triple to collection."""
        if _collection_add_relation(self._coll, s, p, o, d) != 0:
            raise _dlite.DLiteError(
                f'Error adding relation ({s}, {p}, {o}, {d})')

    def remove_relations(self, s=None, p=None, o=None, d=None):
        """Remove all relations matching `s`, `p` and `o`."""
        if _collection_remove_relations(self._coll, s, p, o, d) < 0:
            raise _dlite.DLiteError(
                f'Error removing relations matching ({s}, {p}, {o}, {d})')

    def get_first_relation(self, s=None, p=None, o=None, d=None):
        """Returns the first relation matching `s`, `p` and `o`.
        None is returned if there are no matching relations."""
        return _collection_find_first(self._coll, s, p, o, d)

    def get_instances(self, metaid=None, property_mappings=False,
                      allow_incomplete=False, ureg=None, function_repo=None,
                      **kwargs):
        """Returns a generator over all instances in collection.

        Arguments:
            metaid: If given, only instances of this metadata will be
                returned.
            property_mappings: Whether to also iterate over new instances
                of type `metaid` instantiated from property mappings.
                Hence, if `property_mappings` is true, `metaid` is required.
            allow_incomplete: Allow "incomplete" property mappings.
                Properties with no mappings will be initialised to zero.
            ureg: Optional custom Pint unit registry.  By default the builtin
                Pint unit registry is used.
            function_repo: Repository for mapping functions.  Should map
                function IRIs to function implementations.
            kwargs: Additional keyword arguments passed to mapping_route().
        """
        iter = _CollectionIter(self, s=None, p=None, o=None, rettype='I')
        if metaid:
            if hasattr(metaid, "uri"):
                uri = metaid.uri
            else:
                uri = str(metaid).rstrip("#/")

            for inst in iter:
                if inst.meta.uri == uri:
                    yield inst

            if property_mappings:
                # This import must be here to avoid circular imports
                # -- consider refacturing.
                from dlite.mappings import instantiate_all

                try:
                    import pint
                    from tripper import Namespace, Triplestore
                except ImportError as exc:
                    raise ImportError(
                        "Property mappings require tripper and pint. "
                        "Please do:\n\n"
                        "    pip install pint tripper\n"
                    ) from exc

                meta = metaid if isinstance(
                    metaid, Instance) else _dlite.get_instance(
                        str(metaid).rstrip("#/"))
                if not meta:
                    raise TypeError(f"cannot instantiate metadata: {metaid}")
                if meta.is_meta:
                    meta.__class__ = Metadata

                if ureg is None:
                    quantity = pint.Quantity
                elif isinstance(ureg, pint.UnitRegistry):
                    quantity = ureg.Quantity
                elif isinstance(ureg, pint.Quantity):
                    quantity = ureg
                else:
                    raise TypeError(
                        '`ureg` must be a Pint unit registry or Quantity '
                        'class')

                ts = Triplestore(backend='collection', collection=self)
                if function_repo:
                    ts.function_repo.update(function_repo)

                for inst in instantiate_all(
                    meta=meta,
                    instances=list(self.get_instances()),
                    triplestore=ts,
                    allow_incomplete=allow_incomplete,
                    quantity=quantity,
                    **kwargs
                ):
                    yield inst

        elif property_mappings:
            raise _dlite.DLiteError(
                '`metaid` is required when `property_mappings` is true')
        else:
            for inst in iter:
                yield inst

    def get_labels(self):
        """Returns a generator over all labels."""
        return self.get_subjects(p='_is-a', o='Instance')

    def get_relations(self, s=None, p=None, o=None, d=None, rettype='t'):
        """Returns a generator over all relations matching the given
        values of `s` (subject), `p` (predicate), `o` (object) and `d`
        (datatype). See docstring for dlite.Collection for more info.

        The following values for `rettype` are supported:
        - 'I': Iterate over instances.
        - 'R': Iterate over relation objects.
        - 'T': Iterate over (s,p,o,d) tuples.
        - 't': Iterate over (s,p,o) tuples.
        - 's': Iterate over subjects.
        - 'p': Iterate over predicates.
        - 'o': Iterate over objects.
        - 'd': Iterate over datatypes.
        """
        return _CollectionIter(self, s=s, p=p, o=o, d=d, rettype=rettype)

    def get_subjects(self, p=None, o=None, d=None):
        """Returns a generator over all subjects of relations matching the
        given values of `p`, `o` and `d`."""
        return _CollectionIter(self, s=None, p=p, o=o, d=d, rettype='s')

    def get_predicates(self, s=None, o=None, d=None):
        """Returns a generator over all predicates of relations matching the
        given values of `s`, `o` and `d`."""
        return _CollectionIter(self, s=s, p=None, o=o, d=d, rettype='p')

    def get_objects(self, s=None, p=None, d=None):
        """Returns a generator over all subjects of relations matching the
        given values of `s`, `p` and `d`."""
        return _CollectionIter(self, s=s, p=p, o=None, d=d, rettype='o')

    def value(self, s=None, p=None, o=None, d=None, default=None, any=False):
        """Return pointer to the value for a pair of two criteria.

        Useful if one knows that there may only be one value.

        Parameters:
            s, p, o: Criteria to match. Two of these must not be None.
            d: If not None, the required datatype of literal objects.
            default: Value to return if no matches are found.
            any: If true, return first matching value.

        Returns:
            A pointer to the value of the `s`, `p` or `o` that is None.

        Raises:
            DLiteTypeError: Not exactly two of `s`, `p` and `o` are None.
            DLiteLookupError: No match can be found or more that one match
                if `any` is true.
        """
        return _dlite._collection_value(self, s, p, o, d, default, any)

def get_collection(id):
    """Returns a new reference to a collection with given id."""
    warn("dlite.get_collection() is deprecated.  "
         "Use dlite.get_instance() instead",
         DeprecationWarning, stacklevel=2)
    _coll = _get_collection(id=id)
    inst = _coll.asinstance()
    inst.__class__ = Collection
    return inst

del warn
%}
