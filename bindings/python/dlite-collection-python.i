/* -*- Python -*-  (not really, but good for syntax highlighting) */


%extend struct _CollectionIter {
%pythoncode %{
    def __iter__(self):
        return self

    def __next__(self):
        if self.rettype == 'I':  # instance
            v = self.next()
        elif self.rettype in 'RTspo':  # relation
            v = self.next_relation()
        else:
            raise ValueError('`rettype` must one of "IRTspo"')

        if v is None:
            raise StopIteration()

        if self.rettype == 'T':  # return (s,p,o) tuple
            return (v.s, v.p, v.o)
        elif self.rettype == 's':  # return subject
            return v.s
        elif self.rettype == 'p':  # return predicate
            return v.p
        elif self.rettype == 'o':  # return object
            return v.o
        return v
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

    Use the get_relations() method (or the convenience methods
    get_subjects(), get_predicates() and get_predicates()) to iterate
    over relations.  The number of relations is available via the `nrelations`
    property.
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
            location: str
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
            raise DLiteError(f'No such label in collection: "{label}"')

    def get(self, label, metaid=None):
        """Return instance with given label."""
        return _collection_get_new(self._coll, label, metaid)

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

    def add_relation(self, s, p, o):
        """Add (subject, predicate, object) RDF triple to collection."""
        if _collection_add_relation(self._coll, s, p, o) != 0:
            raise DLiteError(f'Error adding relation ({s}, {p}, {o})')

    def remove_relations(self, s=None, p=None, o=None):
        """Remove all relations matching `s`, `p` and `o`."""
        if _collection_remove_relations(self._coll, s, p, o) < 0:
            raise DLiteError(
                f'Error removing relations matching ({s}, {p}, {o})')

    def get_first_relation(self, s=None, p=None, o=None):
        """Returns the first relation matching `s`, `p` and `o`.
        None is returned if there are no matching relations."""
        return _collection_find_first(self._coll, s, p, o)

    def get_instances(self, metaid=None):
        """Returns a generator over all instances in collection.
        If `metaid` is given, only instances of this metadata will be
        returned.
        """
        # An iterator over all instances
        iter = _CollectionIter(self, s=None, p=None, o=None, rettype='I')
        if metaid:  # `metaid` given - only yield instances of this type
            uri = metaid.uri if hasattr(metaid, "uri") else metaid
            for inst in iter:
                if inst.meta.uri == uri:
                    yield inst
        else:
            for inst in iter:  #  yield all instances
                yield inst

    def get_labels(self):
        """Returns a generator over all labels."""
        return self.get_subjects(p='_is-a', o='Instance')

    def get_relations(self, s=None, p=None, o=None, rettype='T'):
        """Returns a generator over all relations matching the given
        values of `s`, `p` and `o`."""
        return _CollectionIter(self, s=s, p=p, o=o, rettype=rettype)

    def get_subjects(self, p=None, o=None):
        """Returns a generator over all subjects of relations matching the
        given values of `p` and `o`."""
        return _CollectionIter(self, s=None, p=p, o=o, rettype='s')

    def get_predicates(self, s=None, o=None):
        """Returns a generator over all predicates of relations matching the
        given values of `s` and `o`."""
        return _CollectionIter(self, s=s, p=None, o=o, rettype='p')

    def get_objects(self, s=None, p=None):
        """Returns a generator over all subjects of relations matching the
        given values of `s` and `p`."""
        return _CollectionIter(self, s=s, p=p, o=None, rettype='o')


def get_collection(id):
    """Returns a new reference to a collection with given id."""
    warn("dlite.get_collection() is deprecated.  "
         "Use dlite.get_instance() instead",
         DeprecationWarning, stacklevel=2)
    _coll = _get_collection(id=id)
    inst = _coll.asinstance()
    inst.__class__ = Collection
    return inst

%}
