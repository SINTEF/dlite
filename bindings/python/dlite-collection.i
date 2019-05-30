/* -*- C -*-  (not really, but good for syntax highlighting) */

/* --------
 * Iterator
 * -------- */
%rename(CollectionIter) _CollectionIter;
%inline %{
  struct _CollectionIter {
    DLiteCollection *coll;
    DLiteCollectionState state;
  };
%}

%feature("docstring", "\
  Iterator over instances in a collection.
") _CollectionIter;
%extend struct _CollectionIter {
  ~_CollectionIter(void) {
    dlite_collection_deinit_state(&$self->state);
    free($self);
  }

  %feature("docstring", "\
    Returns a reference to next instance.
  ") next;
  struct _DLiteInstance *next(void) {
    return dlite_collection_next($self->coll, &$self->state);
  }

  %feature("docstring", "\
    Returns a reference to next instance matching the provided subject `s`,
    predicate `p` and object `o`.  If any of `s`, `p` or `o` are None, it
    works as a wildcard matching everything.
  ") next;
  const struct _Triplet *find(const char *s=NULL, const char *p=NULL,
                              const char *o=NULL) {
    return dlite_collection_find($self->coll, &$self->state, s, p, o);
  }

  %feature("docstring", "\
    Returns reference to the current instance or None if all instances have
    been visited.
  ") poll;
  const struct _Triplet *poll(void) {
    return triplestore_poll(&$self->state);
  }

  %feature("docstring", "\
    Resets the iterator.  The next call to next() will return the first
    instance.
  ") reset;
  void reset(void) {
    triplestore_reset_state(&$self->state);
  }
}



/* ----------
 * Collection
 * ---------- */
%rename(Collection) _DLiteCollection;
struct _DLiteCollection {
  %immutable;
  char uuid[DLITE_UUID_LENGTH+1];
  char *uri;
  int refcount;
};

%feature("docstring", "\
Returns a collection instance.

Collection(id=None)
    Creates a new empty collection with the given `id`.  The id may be any
    string uniquely identifying this collection.

Collection(storage, id, lazy=0)
    Loads collection with given `id` from `storage`.  If `lazy` is zero,
    all its instances are loaded immediately.  Otherwise, instances are
    first loaded on demand.

Collection(url, lazy)
    Loads collection from `url`, which should be of the form
    ``driver://location?options#id``.  The `lazy` argument has the same
    meaning as above.

") _DLiteCollection;
%extend struct _DLiteCollection {
  _DLiteCollection(const char *id=NULL) {
    return dlite_collection_create(id);
  }
  _DLiteCollection(struct _DLiteStorage *storage, const char *id, int lazy=0) {
    return dlite_collection_load(storage, id, lazy);
  }
  _DLiteCollection(const char *url, int lazy) {
    return dlite_collection_load_url(url, lazy);
  }

  ~_DLiteCollection(void) {
    dlite_collection_decref($self);
  }

  %feature("docstring", "Saves this instance to url or storage.") save;
  void save(struct _DLiteStorage *storage) {
    dlite_collection_save($self, storage);
  }
  void save(const char *url) {
    dlite_collection_save_url($self, url);
  }
  void save(const char *driver, const char *path, const char *options=NULL) {
    DLiteStorage *s;
    if ((s = dlite_storage_open(driver, path, options))) {
      dlite_collection_save($self, s);
      dlite_storage_close(s);
    }
  }

  %feature("docstring",
           "Adds (subject, predicate, object) relation to the collection."
           ) add_relation;
  void add_relation(const char *s, const char *p, const char *o) {
    dlite_collection_add_relation($self, s, p, o);
  }
  void add_relation(const Triplet *t) {
    triplestore_add_triplets($self->rstore, t, 1);
  }

  %feature("docstring", "Returns reference to metadata.") get_meta;
  const struct _DLiteInstance *get_meta() {
    return (const DLiteInstance *)$self->meta;
  }

  %newobject asinstance;
  %feature("docstring",
           "Returns a new view of self as an instance.") as_instance;
  struct _DLiteInstance *asinstance() {
    DLiteInstance *inst = (DLiteInstance *)$self;
    dlite_instance_incref(inst);
    return inst;
  }

  %feature("docstring", "\
  Removes all relations matching the provided subject `s`, predicate `p`
  and object `o`.  If any of `s`, `p` and/or `o` are None, they works as
  a wildcard.
  ") remove_relations;
  void remove_relations(const char *s=NULL, const char *p=NULL,
                        const char *o=NULL) {
    dlite_collection_remove_relations($self, s, p, o);
  }

  %feature("docstring", "\
  Returns the first relation matching the provided subject `s`, predicate
  `p` and object `o`.  If any of `s`, `p` and/or `o` are None, they works
  as a wildcard.
  ") find_first;
  const struct _Triplet *find_first(const char *s=NULL, const char *p=NULL,
                                    const char *o=NULL) {
    return dlite_collection_find_first($self, s, p, o);
  }

  %newobject get_iter;
  %feature("docstring", "\
  Returns an iterator for iterating over instances.
  ") get_iter;
  struct _CollectionIter *get_iter(void) {
    struct _CollectionIter *iter = malloc(sizeof(struct _CollectionIter));
    iter->coll = $self;
    dlite_collection_init_state($self, &iter->state);
    return iter;
  }

  %feature("docstring", "Adds instance `inst` to the collection.") add;
  void add(const char *label, struct _DLiteInstance *inst) {
    dlite_collection_add($self, label, inst);
  }

  %feature("docstring",
           "Removes instance with given label from collection.") remove;
  void remove(const char *label) {
    dlite_collection_remove($self, label);
  }

  %feature("docstring", "\
  Returns a reference to instance with given label or None on error.
  ") get;
  const struct _DLiteInstance *get(const char *label) {
    return dlite_collection_get($self, label);
  }

  %feature("docstring", "\
  Returns a reference to instance with given id or None on error.
  ") get_id;
  const struct _DLiteInstance *get_id(const char *id) {
    return dlite_collection_get_id($self, id);
  }

  %newobject get_new;
  %feature("docstring", "\
  Returns a new instance with given label.  If `metaid` is given, the
  returned instance is casted to this metadata.

  Returns None on error.
  ") get_new;
  const struct _DLiteInstance *get_new(const char *label,
                                       const char *metaid=NULL) {
    return dlite_collection_get_new($self, label, metaid);
  }

  %feature("docstring", "\
  Returns true if the collection contains a reference with the given label.
  ") has;
  bool has(const char *label) {
    return dlite_collection_has($self, label);
  }

  %feature("docstring", "\
  Returns true if the collection contains a reference with the given id
  (URI or UUID).
  ") has_id;
  bool has_id(const char *id) {
    return dlite_collection_has_id($self, id);
  }

  %feature("docstring", "\
  Returns number of instances in the collection.
  ") count;
  int count(void) {
    return dlite_collection_count($self);
  }

}



/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-collection-python.i"
#endif
