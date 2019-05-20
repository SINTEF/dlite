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

%extend struct _CollectionIter {
  ~_CollectionIter(void) {
    dlite_collection_deinit_state(&$self->state);
    free($self);
  }

  struct _DLiteInstance *next(void) {
    return dlite_collection_next($self->coll, &$self->state);
  }

  const struct _Triplet *find(const char *s=NULL, const char *p=NULL,
                              const char *o=NULL) {
    return dlite_collection_find($self->coll, &$self->state, s, p, o);
  }

  const struct _Triplet *poll(void) {
    return triplestore_poll(&$self->state);
  }

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

%extend struct _DLiteCollection {
  _DLiteCollection(const char *id=NULL) {
    return dlite_collection_create(id);
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

  void add_relation(const char *s, const char *p, const char *o) {
    dlite_collection_add_relation($self, s, p, o);
  }
  void add_relation(const Triplet *t) {
    triplestore_add_triplets($self->rstore, t, 1);
  }

  void remove_relations(const char *s=NULL, const char *p=NULL,
                        const char *o=NULL) {
    dlite_collection_remove_relations($self, s, p, o);
  }

  const struct _Triplet *find_first(const char *s=NULL, const char *p=NULL,
                                    const char *o=NULL) {
    return dlite_collection_find_first($self, s, p, o);
  }

  %newobject get_iter;
  struct _CollectionIter *get_iter(void) {
    struct _CollectionIter *iter = malloc(sizeof(struct _CollectionIter));
    iter->coll = $self;
    dlite_collection_init_state($self, &iter->state);
    return iter;
  }

  void add(const char *label, struct _DLiteInstance *inst) {
    dlite_collection_add($self, label, inst);
  }

  void remove(const char *label) {
    dlite_collection_remove($self, label);
  }

  const struct _DLiteInstance *get(const char *label) {
    return dlite_collection_get($self, label);
  }

  const struct _DLiteInstance *get_id(const char *id) {
    return dlite_collection_get_id($self, id);
  }

  const struct _DLiteInstance *get_new(const char *label,
                                       const char *metaid=NULL) {
    return dlite_collection_get_new($self, label, metaid);
  }

  bool has(const char *label) {
    return dlite_collection_has($self, label);
  }

  bool has_id(const char *id) {
    return dlite_collection_has_id($self, id);
  }

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
