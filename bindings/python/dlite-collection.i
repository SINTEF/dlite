/* -*- C -*-  (not really, but good for syntax highlighting) */

/* --------
 * Wrappers
 * -------- */
%{

/* Returns a collection corresponding to `id`. */
struct _DLiteCollection *get_collection(const char *id)
{
  struct _DLiteInstance *inst = dlite_instance_get(id);
  if (!inst) return dlite_err(1, "no instance with this id: %s", id), NULL;
  if (strcmp(inst->meta->uri, DLITE_COLLECTION_ENTITY) != 0)
    return dlite_err(1, "not a collection: %s", id), NULL;
  return (DLiteCollection *)inst;
}

%}


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
  const struct _Triple *find(const char *s=NULL, const char *p=NULL,
                              const char *o=NULL) {
    return dlite_collection_find($self->coll, &$self->state, s, p, o);
  }

  %feature("docstring", "\
    Returns reference to the current instance or None if all instances have
    been visited.
  ") poll;
  const struct _Triple *poll(void) {
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
  int _refcount;
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
  void add_relation(const Triple *t) {
    triplestore_add_triples($self->rstore, t, 1);
  }

  %feature("docstring", "Returns reference to metadata.") get_meta;
  const struct _DLiteInstance *get_meta() {
    return (const DLiteInstance *)$self->meta;
  }

  %newobject asinstance;
  %feature("docstring",
           "Returns a new view of self as an instance.") asinstance;
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
  const struct _Triple *find_first(const char *s=NULL, const char *p=NULL,
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

  %newobject get;
  %feature("docstring", "\
    Returns instance corresponding to `label`.  If `metaid` is provided,
    the returned instance will be mapped to an instance of this metadata.
    Returns NULL on error.
  ") get;
  struct _DLiteInstance *get(const char *label, const char *metaid=NULL) {
    DLiteInstance *inst;
    if (!(inst = dlite_collection_get_new($self, label, metaid)))
      return dlite_err(1, "cannot load \"%s\" from collection", label), NULL;
    return inst;
  }

  %newobject get_id;
  %feature("docstring", "\
  Returns a reference to instance with given id or None on error.
  ") get_id;
  struct _DLiteInstance *get_id(const char *id) {
    DLiteInstance *inst = (DLiteInstance *)dlite_collection_get_id($self, id);
    if (inst) dlite_instance_incref(inst);
    return inst;
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

  %feature("docstring", "\
  Increase reference count and return the new refcount.
  ") incref;
  int incref(void) {
    return dlite_instance_incref((DLiteInstance *)$self);
  }

  %feature("docstring", "\
  Decrease reference count and return the new refcount.
  ") decref;
  int decref(void) {
    return dlite_instance_decref((DLiteInstance *)$self);
  }

}


/* ----------------
 * Module functions
 * ---------------- */
%feature("docstring", "\
Returns a new reference to a collection with given id.
") get_collection;
struct _DLiteCollection *get_collection(const char *id);


/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-collection-python.i"
#endif
