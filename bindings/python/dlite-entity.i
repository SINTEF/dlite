/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
#include "dlite-mapping.h"
%}


/* --------
 * Wrappers
 * -------- */
%{
/* Returns a new property. */
DLiteProperty *
dlite_swig_create_property(const char *name, enum _DLiteType type,
                           size_t size, obj_t *dims, const char *unit,
                           const char *iri,
                           const char *description)
{
  DLiteProperty *p = calloc(1, sizeof(DLiteProperty));
  p->name = strdup(name);
  p->type = type;
  p->size = size;
  if (dims && dims != DLiteSwigNone) {
    p->ndims = (int)PySequence_Length(dims);
    if (!(p->dims = dlite_swig_copy_array(1, &p->ndims, dliteStringPtr,
                                           sizeof(char *), dims))) {
      free(p->name);
      free(p);
      return NULL;
    }
  } else {
    p->ndims = 0;
    p->dims = NULL;
  }
  if (unit) p->unit = strdup(unit);
  if (iri) p->iri = strdup(iri);
  if (description) p->description = strdup(description);
  return p;
}

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

/* Returns instance corresponding to `id`. */
struct _DLiteInstance *
  dlite_swig_get_instance(const char *id, const char *metaid)
{
  struct _DLiteInstance *inst = dlite_instance_get_casted(id, metaid);
  if (!inst) return dlite_err(1, "no instance with this id: %s", id), NULL;
  return inst;
}

%}



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
%feature("docstring", "\
Creates a new property.

Property(name, type, dims=None, unit=None, iri=None, description=None)
    Creates a new property with the provided attributes.

Property(seq)
    Creates a new property from sequence of 6 strings, corresponding to
    `name`, `type`, `dims`, `unit`, `iri` and `description`.  Valid
    values for `dims` are:
      - '' or '[]': no dimensions
      - '<dim1>, <dim2>': list of dimension names
      - '[<dim1>, <dim2>]': list of dimension names

") _DLiteProperty;
%rename(Property) _DLiteProperty;
struct _DLiteProperty {
  char *name;
  /* enum _DLiteType type; */
  size_t size;
  int ndims;
  /* int *dims; */
  char *unit;
  char *iri;
  char *description;
};

%extend _DLiteProperty {
  _DLiteProperty(const char *name, const char *type,
                 obj_t *dims=NULL, const char *unit=NULL,
                 const char *iri=NULL,
                 const char *description=NULL) {
    DLiteType dtype;
    size_t size;
    if (dlite_type_set_dtype_and_size(type, &dtype, &size)) return NULL;
    return dlite_swig_create_property(name, dtype, size, dims, unit, iri,
                                      description);
  }
  ~_DLiteProperty() {
    free($self->name);
    if ($self->dims) free_str_array($self->dims, $self->ndims);
    if ($self->unit) free($self->unit);
    if ($self->iri) free($self->iri);
    if ($self->description) free($self->description);
    free($self);
  }

  %newobject get_type;
  char *get_type(void) {
    return to_typename($self->type, (int)$self->size);
  }
  int get_dtype(void) {
    return $self->type;
  }
  obj_t *get_dims(void) {
    return dlite_swig_get_array(NULL, 1, &$self->ndims,
                                dliteStringPtr, sizeof(char *), $self->dims);
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
  _Triplet(const char *s, const char *p, const char *o, const char *id=NULL) {
    Triplet *t;
    if (!(t =  calloc(1, sizeof(Triplet)))) FAIL("allocation failure");
    if (triplet_set(t, s, p, o, id)) FAIL("cannot set relation");
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

%{
char *triplet_get_id2(const char *s, const char *p, const char *o,
                      const char *namespace) {
  return triplet_get_id(namespace, s, p, o);
}
%}
 %rename(triplet_get_id) triplet_get_id2;
%newobject triplet_get_id;
char *triplet_get_id(const char *s, const char *p, const char *o,
                     const char *namespace=NULL);

void triplet_set_default_namespace(const char *namespace);


/* --------
 * Instance
 * -------- */
%feature("docstring", "\
Returns a new instance.

Instance(metaid, dims, id=None)
    Creates a new instance of metadata `metaid`.  `dims` must be a
    sequence with the size of each dimension. All values initialized
    to zero.  If `id` is None, a random UUID is generated.  Otherwise
    the UUID is derived from `id`.

Instance(url, metaid=NULL)
    Loads the instance from `url`.  The URL should be of the form
    ``driver://location?options#id``.
    If `metaid` is provided, the instance is tried mapped to this
    metadata before it is returned.

Instance(storage, id=None, metaid=NULL)
    Loads the instance from `storage`. `id` is the id of the instance
    in the storage (not required if the storage only contains more one
    instance).
    If `metaid` is provided, the instance is tried mapped to this
    metadata before it is returned.

Instance(driver, location, options, id=None)
    Loads the instance from storage specified by `driver`, `location`
    and `options`. `id` is the id of the instance in the storage (not
    required if the storage only contains more one instance).

Instance(uri, dimensions, properties, description)
    Creates a new metadata entity (instance of entity schema) casted
    to an instance.

") _DLiteInstance;
%apply(int *IN_ARRAY1, int DIM1) {(int *dims, int ndims)};
%apply(int ndimensions, struct _DLiteDimension *dimensions) {
  (int ndimensions, struct _DLiteDimension *dimensions)};
%apply(int nproperties, struct _DLiteProperty *properties) {
  (int nproperties, struct _DLiteProperty *properties)};

%rename(Instance) _DLiteInstance;
struct _DLiteInstance {
  %immutable;
  char uuid[DLITE_UUID_LENGTH+1];
  char *uri;
  int _refcount;
  /* const struct _DLiteMeta *meta; */
};

%extend _DLiteInstance {
  _DLiteInstance(const char *metaid, int *dims, int ndims,
		 const char *id=NULL) {
    DLiteInstance *inst;
    DLiteMeta *meta;
    size_t i, *d, n=ndims;
    if (!(meta = dlite_meta_get(metaid)))
      return dlite_err(1, "cannot find metadata '%s'", metaid), NULL;
    if (n != meta->_ndimensions) {
      dlite_meta_decref(meta);
      return dlite_err(1, "%s has %zu dimensions",
                       metaid, meta->_ndimensions), NULL;
    }
    d = malloc(n * sizeof(size_t));
    for (i=0; i<n; i++) d[i] = dims[i];
    inst = dlite_instance_create(meta, d, id);
    free(d);
    if (inst) dlite_errclr();
    dlite_meta_decref(meta);
    return inst;
  }
  _DLiteInstance(const char *url, const char *metaid=NULL) {
    DLiteInstance *inst2, *inst = dlite_instance_load_url(url);
    if (inst) {
      dlite_errclr();
      if (metaid) {
        inst2 = dlite_mapping(metaid, (const DLiteInstance **)&inst, 1);
        dlite_instance_decref(inst);
        inst = inst2;
      }
    }
    return inst;
  }
  _DLiteInstance(struct _DLiteStorage *storage, const char *id=NULL,
                 const char *metaid=NULL) {
    DLiteInstance *inst = dlite_instance_load_casted(storage, id, metaid);
    if (inst) dlite_errclr();
    return inst;
  }
  _DLiteInstance(const char *driver, const char *location, const char *options,
                 const char *id=NULL) {
    DLiteStorage *s;
    DLiteInstance *inst;
    if (!(s = dlite_storage_open(driver, location, options))) return NULL;
    inst = dlite_instance_load(s, id);
    dlite_storage_close(s);
    if (inst) dlite_errclr();
    return inst;
  }
  _DLiteInstance(const char *uri,
                 int ndimensions, struct _DLiteDimension *dimensions,
                 int nproperties, struct _DLiteProperty *properties,
                 const char *iri=NULL,
                 const char *description=NULL) {
    DLiteMeta *inst = dlite_entity_create(uri, iri, description,
                                          ndimensions, dimensions,
                                          nproperties, properties);
    if (inst) dlite_errclr();
    return (DLiteInstance *)inst;
  }

  ~_DLiteInstance() {
    dlite_instance_decref($self);
  }

  %feature("docstring", "Returns reference to metadata.") get_meta;
  const struct _DLiteInstance *get_meta() {
    return (const DLiteInstance *)$self->meta;
  }

  %feature("docstring", "Returns ontology IRI reference.") get_iri;
  const char *get_iri() {
    return $self->iri;
  }

  %feature("docstring", "Sets ontology IRI (no argument clears the "
           "IRI).") set_iri;
  void set_iri(const char *iri=NULL) {
    if ($self->iri) free((char *)self->iri);
    $self->iri = (iri) ? strdup(iri) : NULL;
  }

  %feature("docstring", "Saves this instance to url or storage.") save;
  void save(const char *url) {
    dlite_instance_save_url(url, $self);
  }
  void save(struct _DLiteStorage *storage) {
    dlite_instance_save(storage, $self);
  }
  void save(const char *driver, const char *path, const char *options=NULL) {
    DLiteStorage *s;
    if ((s = dlite_storage_open(driver, path, options))) {
      dlite_instance_save(s, $self);
      dlite_storage_close(s);
    }
  }

  %feature("docstring", "Returns array with dimension sizes.") get_dimensions;
  %newobject get_dimensions;
  obj_t *get_dimensions() {
    int dims[1] = { (int)DLITE_NDIM($self) };
    return dlite_swig_get_array($self, 1, dims, dliteUInt, sizeof(size_t),
                                DLITE_DIMS($self));
  }

  %feature("docstring",
           "Returns the size of dimension with given name or index.")
     get_dimension_size;
  int get_dimension_size(const char *name) {
    return (int)dlite_instance_get_dimension_size($self, name);
  }
  int get_dimension_size(int i) {
    return (int)dlite_instance_get_dimension_size_by_index($self, i);
  }

  %feature("docstring", "Returns property with given name or index.")
     get_property;
  %newobject get_property;
  obj_t *get_property(const char *name) {
    return dlite_swig_get_property($self, name);
  }
  obj_t *get_property(int i) {
    return dlite_swig_get_property_by_index($self, i);
  }

  %feature("docstring", "Sets property with given name or index to `obj`.")
     set_property;
  void set_property(const char *name, obj_t *obj) {
    dlite_swig_set_property($self, name, obj);
  }
  void set_property(int i, obj_t *obj) {
    dlite_swig_set_property_by_index($self, i, obj);
  }

  %feature("docstring", "Returns true if this instance has a property with "
           "given name or index.") has_property;
  bool has_property(const char *name) {
    return dlite_instance_has_property($self, name);
  }
  bool has_property(int i) {
    if (i < 0) i += (int)$self->meta->_nproperties;
    if (0 <= i && i < (int)$self->meta->_nproperties) return true;
    return false;
  }

  %feature("docstring", "Returns true if this instance has a dimension with "
           "given name or index.") has_dimension;
  bool has_dimension(const char *name) {
    return dlite_instance_has_dimension($self, name);
  }
  bool has_dimension(int i) {
    if (i < 0) i += (int)$self->meta->_ndimensions;
    if (0 <= i && i < (int)$self->meta->_ndimensions) return true;
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
//%rename(get_instance) dlite_instance_get_casted;
//%newobject dlite_instance_get_casted;
//struct _DLiteInstance *dlite_instance_get_casted(const char *id,
//                                                 const char *metaid=NULL);

%rename(get_instance) dlite_swig_get_instance;
%rename(_get_property) dlite_swig_get_property;
%rename(_set_property) dlite_swig_set_property;
%rename(_has_property) dlite_instance_has_property;
struct _DLiteInstance *
dlite_swig_get_instance(const char *id, const char *metaid=NULL);
obj_t *dlite_swig_get_property(struct _DLiteInstance *inst, const char *name);
void dlite_swig_set_property(struct _DLiteInstance *inst, const char *name,
                             obj_t *obj);
bool dlite_instance_has_property(struct _DLiteInstance *inst, const char *name);

/* FIXME - how do we avoid duplicating these constants from dlite-schemas.h? */
#define BASIC_METADATA_SCHEMA  "http://meta.sintef.no/0.1/BasicMetadataSchema"
#define ENTITY_SCHEMA          "http://meta.sintef.no/0.3/EntitySchema"
#define COLLECTION_SCHEMA      "http://meta.sintef.no/0.6/CollectionSchema"

/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-entity-python.i"
#endif
