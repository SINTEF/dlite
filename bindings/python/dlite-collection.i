/* -*- C -*-  (not really, but good for syntax highlighting) */

/* --------
 * Wrappers
 * -------- */
%{

/* Returns a collection corresponding to `id`. */
struct _DLiteCollection *_get_collection(const char *id)
{
  struct _DLiteInstance *inst = dlite_instance_get(id);
  if (!inst) return dlite_err(dliteTypeError,
                              "no instance with this id: %s", id), NULL;
  if (strcmp(inst->meta->uri, DLITE_COLLECTION_ENTITY) != 0)
    return dlite_err(1, "not a collection: %s", id), NULL;
  return (DLiteCollection *)inst;
}

char *_collection_value(DLiteInstance *inst,
                        const char *s, const char *p,
                        const char *o, const char *d,
                        const char *fallback, int any) {
  if (!inst) return dlite_err(dliteTypeError,
                              "first argument must be provided"), NULL;
  if (strcmp(inst->meta->uri, DLITE_COLLECTION_ENTITY) != 0)
    return dlite_err(dliteTypeError,
                     "first argument must be a collection"), NULL;
  DLiteCollection *coll = (DLiteCollection *)inst;
  const char *v = triplestore_value(coll->rstore, s, p, o, d, fallback, any);
  return (v) ? strdup(v) : NULL;
 }

%}


/* -----------------
 * SWIG declarations
 * ----------------- */

int dlite_collection_save(struct _DLiteCollection *coll,
                          struct _DLiteStorage *s);
int dlite_collection_save_url(struct _DLiteCollection *coll, const char *url);
int dlite_collection_add_relation(struct _DLiteCollection *coll, const char *s,
                                  const char *p, const char *o,
                                  const char *d=NULL);
int dlite_collection_remove_relations(struct _DLiteCollection *coll,
                                      const char *s, const char *p,
                                      const char *o, const char *d=NULL);
const struct _Triple *
  dlite_collection_find_first(const struct _DLiteCollection *coll,
                              const char *s, const char *p, const char *o,
                              const char *d=NULL);
int dlite_collection_add(struct _DLiteCollection *coll, const char *label,
                         struct _DLiteInstance *inst);
int dlite_collection_remove(struct _DLiteCollection *coll, const char *label);
%newobject dlite_collection_get_new;
struct _DLiteInstance *
dlite_collection_get_new(const struct _DLiteCollection *coll,
                         const char *label, const char *metaid);

// Although dlite_collection_get_id() returns a borrowed reference in C,
// we create a new object in Python that must be properly deallocated.
%newobject dlite_collection_get_id;
const struct _DLiteInstance *
  dlite_collection_get_id(const struct _DLiteCollection *coll, const char *id);

int dlite_collection_has(const struct _DLiteCollection *coll,
                         const char *label);
int dlite_collection_has_id(const struct _DLiteCollection *coll,
                            const char *id);
int dlite_collection_count(struct _DLiteCollection *coll);



/* -----------
 * _Collection
 * ----------- */
%rename(_Collection) _DLiteCollection;
struct _DLiteCollection {
  %immutable;
  char uuid[DLITE_UUID_LENGTH+1];
  char *uri;
  int _refcount;
};

%extend struct _DLiteCollection {
  _DLiteCollection(const char *url=NULL, struct _DLiteStorage *storage=NULL,
                   const char *id=NULL, bool lazy=0) {
    if (url) {
      return dlite_collection_load_url(url, lazy);
    } else if (storage) {
      return dlite_collection_load(storage, id, lazy);
    } else {
      return dlite_collection_create(id);
    }
  }

  ~_DLiteCollection(void) {
    dlite_collection_decref($self);
  }

  %newobject asinstance;
  %feature("docstring",
           "Returns a new view of collection as an instance.") asinstance;
  struct _DLiteInstance *asinstance() {
    DLiteInstance *inst = (DLiteInstance *)$self;
    dlite_instance_incref(inst);
    return inst;
  }

}


/* ---------------
 * _CollectionIter
 * --------------- */
%rename(_CollectionIter) _CollectionIter;
%inline %{
  struct _CollectionIter {
    DLiteCollection *coll;
    DLiteCollectionState state;
    char *s, *p, *o, *d;  /* search pattern */
    char rettype;         /* return type: I=Instance; R=Relation;
                             T=(s,p,o,d)-tuple; t=(s,p,o)-tuple;
                             s=subject; p=predicate; o=object; d=datatype */

  };
%}

%feature("docstring",
         "Iterator over instances in a collection."
         ) _CollectionIter;
%extend struct _CollectionIter {
  _CollectionIter(struct _DLiteInstance *inst,
                  const char *s=NULL, const char *p=NULL, const char *o=NULL,
                  const char *d=NULL, const char rettype='t') {
    if (strcmp(inst->meta->uri, DLITE_COLLECTION_ENTITY) != 0)
      return dlite_err(1, "not a collection: %s", inst->uuid), NULL;
    struct _CollectionIter *iter = calloc(1, sizeof(struct _CollectionIter));
    DLiteCollection *coll = (DLiteCollection *)inst;
    iter->coll = coll;
    iter->s = (s) ? strdup(s) : NULL;
    iter->p = (p) ? strdup(p) : NULL;
    iter->o = (o) ? strdup(o) : NULL;
    iter->d = (d) ? strdup(d) : NULL;
    iter->rettype = rettype;
    dlite_collection_init_state(coll, &iter->state);
    return iter;
  }

  ~_CollectionIter(void) {
    dlite_collection_deinit_state(&$self->state);
    if ($self->s) free($self->s);
    if ($self->p) free($self->p);
    if ($self->o) free($self->o);
    if ($self->d) free($self->d);
    free($self);
  }

  %feature("docstring",
           "Returns a reference to next matching relation."
           ) next;
  const struct _DLiteInstance *next(void) {
    return dlite_collection_next_new($self->coll, &$self->state);
  }

  %feature("docstring",
           "Returns a reference to next matching relation."
           ) next;
  const struct _Triple *next_relation(void) {
    return dlite_collection_find($self->coll, &$self->state,
                                 $self->s, $self->p, $self->o, $self->d);
  }

  %feature("docstring",
           "Returns reference to the current instance or None if all "
           "instances have been visited.") poll;
  const struct _Triple *poll(void) {
    return triplestore_poll(&$self->state);
  }

  %feature("docstring",
           "Resets the iterator.  The next call to next() will return the "
           "first relation.") reset;
  void reset(void) {
    triplestore_reset_state(&$self->state);
  }
}



/* ----------------
 * Module functions
 * ---------------- */
%feature("docstring",
         "Returns a new reference to a collection with given id."
         ) _get_collection;
%newobject _get_collection;
struct _DLiteCollection *_get_collection(const char *id);

%feature("docstring", "Return the value for a pair of two criteria.") _value;
%newobject _value;
char *_collection_value(struct _DLiteInstance *inst,
                        const char *s=NULL, const char *p=NULL,
                        const char *o=NULL, const char *d=NULL,
                        const char *fallback=NULL, int any=0);


/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-collection-python.i"
#endif
