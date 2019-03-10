/* -*- C -*-  (not really, but good for syntax highlighting) */


/* --------
 * Wrappers
 * -------- */
%{
/* Returns a pointer to a new target language object for property `name`.
   Returns NULL on error. */
obj_t *dlite_swig_get_property(DLiteInstance *inst, const char *name)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return NULL;
  return dlite_swig_get_property_by_index(inst, i);
}

/* Sets property `name` to the value pointed to by the target language object
   `obj`.   Returns non-zero on error. */
int dlite_swig_set_property(DLiteInstance *inst, const char *name, obj_t *obj)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return -1;
  return dlite_swig_set_property_by_index(inst, i, obj);
}
%}


/* -----
 * Types
 * ----- */
%rename(Type) DLiteType;
%rename("%(regex:/dlite(.*)/\\1Type/)s", %$isenumitem) "";
enum _DLiteType {
  dliteBlob,             /*!< Binary blob, sequence of bytes */
  dliteBool,             /*!< Boolean */
  dliteInt,              /*!< Signed integer */
  dliteUInt,             /*!< Unigned integer */
  dliteFloat,            /*!< Floating point */
  dliteFixString,        /*!< Fix-sized NUL-terminated string */
  dliteStringPtr,        /*!< Pointer to NUL-terminated string */

  dliteDimension,        /*!< Dimension, for entities */
  dliteProperty,         /*!< Property, for entities */
  dliteRelation,         /*!< Subject-predicate-object relation */
};


/* ---------
 * Dimension
 * --------- */

%rename(Dimension) _DLiteDimension;
struct _DLiteDimension {
  char *name;
  char *description;
};

%extend _DLiteDimension {
  _DLiteDimension(const char *name, const char *description=NULL) {
    DLiteDimension *d = calloc(1, sizeof(DLiteDimension));
    d->name = strdup(name);
    if (description) d->description = strdup(description);
    return d;
  }

  ~_DLiteDimension() {
    free($self->name);
    if ($self->description) free($self->description);
    free($self);
  }
}


/* --------
 * Property
 * -------- */
//%apply(int DIM1, char **IN_ARRAY1) {(int ndims, char **dimnames)};
%apply(int DIM1, int *IN_ARRAY1) {(int ndims, int *dims)};

%rename(Property) _DLiteProperty;
struct _DLiteProperty {
  char *name;
  enum _DLiteType type;
  size_t size;
  int ndims;
  /* int *dims; */
  char *unit;
  char *description;
};

%extend _DLiteProperty {
  _DLiteProperty(const char *name, enum _DLiteType type, int size,
                 obj_t *dims=NULL,
                 const char *unit=NULL, const char *description=NULL) {
    DLiteProperty *p = calloc(1, sizeof(DLiteProperty));
    p->name = strdup(name);
    p->type = type;
    p->size = size;
    if (dims && dims != DLiteSwigNone) {
      if (!(p->dims = dlite_swig_copy_array(1, &p->ndims, dliteInt,
                                            sizeof(int), dims))) {
        free(p->name);
        free(p);
        return NULL;
      }
    } else {
      p->ndims = 0;
      p->dims = NULL;
    }
    if (unit) p->unit = strdup(unit);
    if (description) p->description = strdup(description);
    return p;
  }

  ~_DLiteProperty() {
    free($self->name);
    if ($self->dims) free($self->dims);
    if ($self->unit) free($self->unit);
    if ($self->description) free($self->description);
    free($self);
  }

  obj_t *get_dims(void) {
    return dlite_swig_get_array(NULL, 1, &$self->ndims,
                                dliteInt, sizeof(int), $self->dims);
  }
  /*
  void set_dims(obj_t *arr) {
  }
  */
}

/* --------
 * Relation
 * -------- */
%rename(Relation) _Triplet;
struct _Triplet {
  char *s;     /*!< subject */
  char *p;     /*!< predicate */
  char *o;     /*!< object */
  char *id;    /*!< unique ID identifying this triplet */
};

%extend _Triplet {
  _Triplet(const char *s, const char *p, const char *o) {
    Triplet *t;
    if (!(t =  malloc(sizeof(Triplet)))) FAIL("allocation failure");
    if (triplet_set(t, s, p, o, NULL)) FAIL("cannot set relation");
    return t;
  fail:
    if (t) {
      triplet_clean(t);
      free(t);
    }
    return NULL;
  }

  ~_Triplet() {
    triplet_clean($self);
    free($self);
  }
}


/* --------
 * Instance
 * -------- */

//%apply(int DIM1, int *IN_ARRAY1) {(int ndims, int *dims)};

%rename(Instance) _DLiteInstance;
struct _DLiteInstance {
  %immutable;
  char uuid[DLITE_UUID_LENGTH+1];
  char *uri;
  int refcount;
  /* const struct _DLiteMeta *meta; */
};

%extend _DLiteInstance {
  _DLiteInstance(const char *metaid, int ndims, int *dims,
		 const char *id=NULL) {
    DLiteInstance *inst;
    DLiteMeta *meta;
    size_t i, *d, n=ndims;
    if (!(meta = dlite_meta_get(metaid)))
      return dlite_err(1, "cannot find metadata '%s'", metaid), NULL;
    if (n != meta->ndimensions)
      return dlite_err(1, "%s has %zu dimensions",
		       metaid, meta->ndimensions), NULL;
    d = malloc(n * sizeof(size_t));
    for (i=0; i<n; i++) d[i] = dims[i];
    inst = dlite_instance_create(meta, d, id);
    free(d);
    if (inst) dlite_errclr();
    return inst;
  }
  _DLiteInstance(const char *url) {
    DLiteInstance *inst = dlite_instance_load_url(url);
    if (inst) dlite_errclr();
    return inst;
  }
  _DLiteInstance(struct _DLiteStorage *storage, const char *id) {
    DLiteInstance *inst = dlite_instance_load(storage, id);
    if (inst) dlite_errclr();
    return inst;
  }

  ~_DLiteInstance() {
    dlite_instance_decref($self);
  }

  const struct _DLiteInstance *get_meta() {
    return (const DLiteInstance *)$self->meta;
  }

  void save_url(const char *url) {
    dlite_instance_save_url(url, $self);
  }

  %newobject get_dimensions;
  obj_t *get_dimensions() {
    int dims[1] = { DLITE_NDIM($self) };
    return dlite_swig_get_array($self, 1, dims, dliteUInt, sizeof(size_t),
                                DLITE_DIMS($self));
  }

  %newobject get_property;
  obj_t *get_property(const char *name) {
    return dlite_swig_get_property($self, name);
  }
  obj_t *get_property(int i) {
    return dlite_swig_get_property_by_index($self, i);
  }

  void set_property(const char *name, obj_t *obj) {
    dlite_swig_set_property($self, name, obj);
  }
  void set_property(int i, obj_t *obj) {
    dlite_swig_set_property_by_index($self, i, obj);
  }

  bool has_property(const char *name) {
    return dlite_instance_has_property($self, name);
  }
  bool has_property(int i) {
    if (i < 0) i += $self->meta->nproperties;
    if (0 <= i && i < (int)$self->meta->nproperties) return true;
    return false;
  }

  bool _is_data() {
    return (bool)dlite_instance_is_data($self);
  }

  bool _is_meta() {
    return (bool)dlite_instance_is_meta($self);
  }

  bool _is_metameta() {
    return (bool)dlite_instance_is_metameta($self);
  }
};


/* ----------------
 * Module functions
 * ---------------- */
%rename(get_instance) dlite_instance_get;
struct _DLiteInstance *dlite_instance_get(const char *id);

%rename(_get_property) dlite_swig_get_property;
%rename(_set_property) dlite_swig_set_property;
%rename(_has_property) dlite_instance_has_property;
obj_t *dlite_swig_get_property(struct _DLiteInstance *inst, const char *name);
void dlite_swig_set_property(struct _DLiteInstance *inst, const char *name,
                             obj_t *obj);
bool dlite_instance_has_property(struct _DLiteInstance *inst, const char *name);


/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-entity-python.i"
#endif
