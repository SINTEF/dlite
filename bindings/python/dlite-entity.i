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
  dlite_swig_get_instance(const char *id, const char *metaid,
                          bool check_storages)
{
  struct _DLiteInstance *inst=NULL;
  if (check_storages) {
    inst = dlite_instance_get_casted(id, metaid);
  } else if ((inst = dlite_instance_has(id, check_storages))) {
    if (metaid)
      inst = dlite_mapping(metaid, (const DLiteInstance **)&inst, 1);
    else
      dlite_instance_incref(inst);
  }
  return inst;
}

/* Returns true if instance is recognised. */
bool dlite_swig_has_instance(const char *id, bool check_storages)
{
  return (dlite_instance_has(id, check_storages)) ? 1 : 0;
}

/* Returns list of uuids from istore. */
char** dlite_swig_istore_get_uuids()
{
  int n;
  char** uuids;
  uuids = dlite_istore_get_uuids(&n);
  return uuids;
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

Property(name, type, dims=None, unit=None, description=None)
    Creates a new property with the provided attributes.

Property(seq)
    Creates a new property from sequence of 6 strings, corresponding to
    `name`, `type`, `dims`, `unit` and `description`.  Valid
    values for `dims` are:
      - '' or '[]': no dimensions
      - '<dim1>, <dim2>': list of dimension names
      - '[<dim1>, <dim2>]': list of dimension names

") _DLiteProperty;
%rename(Property) _DLiteProperty;
struct _DLiteProperty {
  char *name;
  size_t size;
  int ndims;
  char *unit;
  char *description;
};

%extend _DLiteProperty {
  _DLiteProperty(const char *name, const char *type,
                 obj_t *dims=NULL, const char *unit=NULL,
                 const char *description=NULL) {
    DLiteType dtype;
    size_t size;
    if (dlite_type_set_dtype_and_size(type, &dtype, &size)) return NULL;
    return dlite_swig_create_property(name, dtype, size, dims, unit,
                                      description);
  }
  ~_DLiteProperty() {
    free($self->name);
    if ($self->dims) free_str_array($self->dims, $self->ndims);
    if ($self->unit) free($self->unit);
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
%rename(Relation) _Triple;
struct _Triple {
  char *s;     /*!< subject */
  char *p;     /*!< predicate */
  char *o;     /*!< object */
  char *id;    /*!< unique ID identifying this triple */
};

%extend _Triple {
  _Triple(const char *s, const char *p, const char *o, const char *id=NULL) {
    Triple *t;
    if (!(t =  calloc(1, sizeof(Triple)))) FAIL("allocation failure");
    if (triple_set(t, s, p, o, id)) FAIL("cannot set relation");
    return t;
  fail:
    if (t) {
      triple_clean(t);
      free(t);
    }
    return NULL;
  }

  ~_Triple() {
    triple_clean($self);
    free($self);
  }
}

%{
char *triple_get_id2(const char *s, const char *p, const char *o,
                      const char *namespace) {
  return triple_get_id(namespace, s, p, o);
}
%}
 %rename(triple_get_id) triple_get_id2;
%newobject triple_get_id;
char *triple_get_id(const char *s, const char *p, const char *o,
                     const char *namespace=NULL);

void triple_set_default_namespace(const char *namespace);


/* --------
 * Instance
 * -------- */
%feature("docstring", "\
Returns a new instance.

Instance(metaid=None, dims=None, id=None, url=None, storage=None, driver=None, location=None, options=None, dimensions=None, properties=None, description=None)
    Is called from one of the following class methods defined in dlite.py:

      - from_metaid(cls, metaid, dims, id=None)
      - from_url(cls, url, metaid=None)
      - from_storage(cls, storage, id=None, metaid=None)
      - from_location(cls, driver, location, options=None, id=None)
      - from_json(cls, jsoninput, id=None, metaid=None)
      - create_metadata(cls, uri, dimensions, properties, description)

      For details, see the documentation for the class methods.

") _DLiteInstance;
%apply(int *IN_ARRAY1, int DIM1) {(int *dims, int ndims)};
%apply(struct _DLiteDimension *dimensions, int ndimensions) {
  (struct _DLiteDimension *dimensions, int ndimensions)};
%apply(struct _DLiteProperty *properties, int nproperties) {
  (struct _DLiteProperty *properties, int nproperties)};

%rename(Instance) _DLiteInstance;
struct _DLiteInstance {
  %immutable;
  char uuid[DLITE_UUID_LENGTH+1];
  unsigned char _flags;
  char *uri;
  int _refcount;
  /* const struct _DLiteMeta *meta; */
};

%extend _DLiteInstance {
  _DLiteInstance(const char *metaid=NULL, int *dims=NULL, int ndims=0,
                 const char *id=NULL, const char *url=NULL,
                 struct _DLiteStorage *storage=NULL,
                 const char *driver=NULL, const char *location=NULL,
                 const char *options=NULL,
                 const char *uri=NULL,
                 const char *jsoninput=NULL,
                 struct _DLiteDimension *dimensions=NULL, int ndimensions=0,
                 struct _DLiteProperty *properties=NULL, int nproperties=0,
                 const char *description=NULL) {
    if (dims && metaid) {
      DLiteInstance *inst;
      DLiteMeta *meta;
      size_t i, *d, n=ndims;
      if (!(meta = dlite_meta_get(metaid)))
        return dlite_err(1, "cannot find metadata '%s'", metaid), NULL;
      if (n != meta->_ndimensions) {
        dlite_meta_decref(meta);
        return dlite_err(1, "%s has %u dimensions",
                          metaid, (unsigned)meta->_ndimensions), NULL;
      }
      d = malloc(n * sizeof(size_t));
      for (i=0; i<n; i++) d[i] = dims[i];
      inst = dlite_instance_create(meta, d, id);
      free(d);
      if (inst) dlite_errclr();
      dlite_meta_decref(meta);
      return inst;
    } else if (url) {
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
    } else if (storage) {
      DLiteInstance *inst = dlite_instance_load_casted(storage, id, metaid);
      if (inst) dlite_errclr();
      return inst;
    } else if (driver && location) {
      DLiteStorage *s;
      DLiteInstance *inst;
      if (!(s = dlite_storage_open(driver, location, options))) return NULL;
      inst = dlite_instance_load(s, id);
      dlite_storage_close(s);
      if (inst) dlite_errclr();
      return inst;
    } else if (jsoninput) {
      DLiteInstance *inst = dlite_json_sscan(jsoninput, id, metaid);
      if (inst) dlite_errclr();
      return inst;
    } else if (uri && dimensions && properties && description){
       DLiteMeta *inst = dlite_meta_create(uri, description,
                                        ndimensions, dimensions,
                                        nproperties, properties);
      if (inst) dlite_errclr();
      return (DLiteInstance *)inst;
    } else {
      dlite_err(1, "invalid arguments to Instance()");
    }
    return NULL;
  }

  ~_DLiteInstance() {
    dlite_instance_decref($self);
  }

  %feature("docstring", "Returns reference to metadata.") _get_meta;
  const struct _DLiteInstance *_get_meta() {
    return (const DLiteInstance *)$self->meta;
  }

  %feature("docstring", "\
Saves this instance to url or storage.

Call signatures:
    save(url)
    save(driver, location, options=None)
    save(storage=storage)
") save;
  void save(const char *driver_or_url=NULL, const char *location=NULL,
            const char *options=NULL, struct _DLiteStorage *storage=NULL) {
    if (storage) {
      dlite_instance_save(storage, $self);
    } else if (location) {
      if ((storage = dlite_storage_open(driver_or_url, location, options))) {
        dlite_instance_save(storage, $self);
        dlite_storage_close(storage);
      }
    } else if (driver_or_url) {
      dlite_instance_save_url(driver_or_url, $self);
    } else {
      dlite_err(-1, "either `driver_or_url` or `storage` must be given");
    }
  }
  void save_to_url(const char *url) {
    dlite_instance_save_url(url, $self);
  }
  void save_to_storage(struct _DLiteStorage *storage) {
    dlite_instance_save(storage, $self);
  }

  %feature("docstring", "Returns a hash of instance.") get_hash;
  %newobject get_hash;
  char *get_hash() {
    uint8_t hash[DLITE_HASH_SIZE];
    char *hex;
    if (dlite_instance_get_hash($self, hash, DLITE_HASH_SIZE))
      return NULL;
    if (!(hex = malloc(2*DLITE_HASH_SIZE+1)))
      return dlite_err(1, "allocation failure"), NULL;
    if (strhex_encode(hex, 2*DLITE_HASH_SIZE+1, hash, DLITE_HASH_SIZE) < 0) {
      dlite_err(1, "failed hex-encoding hash of '%s'", $self->uuid);
      free(hex);
      return NULL;
    }
    return hex;
  }

  %feature("docstring",
           "Returns an copy of instance.  If newid is given, it will be "
           "the id of the new instance, otherwise it will be given a "
           "random UUID.") get_copy;
  %newobject get_copy;
  struct _DLiteInstance *get_copy(const char *newid=NULL) {
    return dlite_instance_copy($self, newid);
  }

  %feature("docstring",
           "Returns an immutable snapshot of instance.") get_snapshot;
  %newobject get_snapshot;
  struct _DLiteInstance *get_snapshot() {
    return dlite_get_snapshot($self);
  }

  %feature("docstring", "Returns array with dimension sizes.") get_dimensions;
  %newobject get_dimensions;
  obj_t *get_dimensions() {
    int dims[1] = { (int)DLITE_NDIM($self) };
    dlite_instance_sync_to_dimension_sizes($self);
    return dlite_swig_get_array($self, 1, dims, dliteUInt, sizeof(size_t),
                                DLITE_DIMS($self));
  }

  %feature("docstring",
           "Returns the size of dimension with given name or index.")
     get_dimension_size;
  int get_dimension_size(const char *name) {
    return (int)dlite_instance_get_dimension_size($self, name);
  }
  int get_dimension_size_by_index(int i) {
    return (int)dlite_instance_get_dimension_size_by_index($self, i);
  }

  %feature("docstring", "Returns property with given name.")
     get_property;
  %newobject get_property;
  obj_t *get_property(const char *name) {
    return dlite_swig_get_property($self, name);
  }
  %feature("docstring", "Returns property with given index.")
     get_property_by_index;
  obj_t *get_property_by_index(int i) {
    return dlite_swig_get_property_by_index($self, i);
  }

  %feature("docstring", "Sets property with given name to `obj`.")
     set_property;
  void set_property(const char *name, obj_t *obj) {
    dlite_swig_set_property($self, name, obj);
  }
  %feature("docstring", "Sets property with given index to `obj`.")
     set_property_by_index;
  void set_property_by_index(int i, obj_t *obj) {
    dlite_swig_set_property_by_index($self, i, obj);
  }

  %feature("docstring",
           "Return property `name` as a string.\n"
           "\n"
           "`width`  Minimum field width. Unused if 0, auto if -1.\n"
           "`prec`   Precision. Auto if -1, unused if -2.\n"
           "`flags`  Or'ed sum of formatting flags:\n"
           "    0  default (json)\n"
           "    1  raw unquoted output\n"
           "    2  quoted output")
     get_property_as_string;
  %newobject get_property_as_string;
  char *get_property_as_string(const char *name,
                               int width=0, int prec=-2, int flags=0) {
    char *dest=NULL;
    size_t n=0;
    if (dlite_instance_aprint_property(&dest, &n, 0, $self, name, width,
                                       prec, flags) < 0) {
      if (dest) free(dest);
      dest = NULL;
    }
    return dest;
  }
  %feature("docstring",
           "Set property `name` to the value of string `s`. \n"
           "\n"
           "`flags` is the or'ed sum of:\n"
           "  0  default (json)\n"
           "  1  raw unquoted input\n"
           "  2  quoted input\n"
           "  4  strip initial and final spaces")
     set_property_from_string;
  void set_property_from_string(const char *name, const char *s, int flags=0) {
    dlite_instance_scan_property(s, $self, name, flags);
  }

  %feature("docstring", "Returns true if this instance has a property with "
           "given name.") has_property;
  bool has_property(const char *name) {
    return dlite_instance_has_property($self, name);
  }
  %feature("docstring", "Returns true if this instance has a property with "
           "given index.") has_property_by_index;
  bool has_property_by_index(int i) {
    if (i < 0) i += (int)$self->meta->_nproperties;
    if (0 <= i && i < (int)$self->meta->_nproperties) return true;
    return false;
  }

  %feature("docstring", "Returns true if this instance has a dimension with "
           "given name.") has_dimension;
  bool has_dimension(const char *name) {
    return dlite_instance_has_dimension($self, name);
  }
  %feature("docstring", "Returns true if this instance has a dimension with "
           "given index.") has_dimension_by_index;
  bool has_dimension_by_index(int i) {
    if (i < 0) i += (int)$self->meta->_ndimensions;
    if (0 <= i && i < (int)$self->meta->_ndimensions) return true;
    return false;
  }

  %feature("docstring", "Returns true if this is a data instance.") _is_data;
  bool _is_data(void) {
    return (bool)dlite_instance_is_data($self);
  }

  %feature("docstring", "Returns true if this is metadata.") _is_meta;
  bool _is_meta(void) {
    return (bool)dlite_instance_is_meta($self);
  }

  %feature("docstring", "Returns true if this is meta-metadata.") _is_metameta;
  bool _is_metameta(void) {
    return (bool)dlite_instance_is_metameta($self);
  }

  %feature("docstring",
           "Increase reference count and return the new refcount.") _incref;
  int _incref(void) {
    return dlite_instance_incref($self);
  }

  %feature("docstring",
           "Decrease reference count and return the new refcount.") _decref;
  int _decref(void) {
    return dlite_instance_decref($self);
  }

  %feature("docstring",
           "") tojson;
  %newobject tojson;
  char *tojson(int indent=0, int flags=0) {
    return dlite_json_aprint($self, indent, flags);
  }


};



/* ----------------
 * Module functions
 * ---------------- */

%feature("docstring", "\
Returns a new reference to instance with given id.

If `metaid` is provided, the instance will be mapped to an instance of
this metadata.

If the instance exists in the in-memory store it is returned.
Otherwise, if `check_storages` is true, the instance is searched for
in the storage plugin path (initiated from the DLITE_STORAGES
environment variable).

It is an error message if the instance cannot be found.

Note: seting `check_storages` to false is normally a good idea if calling
this function from a storage plugin.  Otherwise you may easily end up in an
infinite recursive loop that will exhaust the call stack.
") dlite_swig_get_instance;
%rename(get_instance) dlite_swig_get_instance;
%newobject dlite_swig_get_instance;
struct _DLiteInstance *
dlite_swig_get_instance(const char *id, const char *metaid=NULL,
                        bool check_storages=true);


%feature("docstring", "\
Returns whether an instance with `id` exists.

If `check_storages` is true, the instance is also searched for
in the storage plugin path.
") dlite_swig_has_instance;
%rename(has_instance) dlite_swig_has_instance;
bool dlite_swig_has_instance(const char *id, bool check_storages=true);


%feature("docstring", "\
Returns a list of in-memory stored ids.

") dlite_swig_istore_get_uuids;
%rename(istore_get_uuids) dlite_swig_istore_get_uuids;
char** dlite_swig_istore_get_uuids();

%rename(_get_property) dlite_swig_get_property;
%rename(_set_property) dlite_swig_set_property;
%rename(_has_property) dlite_instance_has_property;
obj_t *dlite_swig_get_property(struct _DLiteInstance *inst, const char *name);
void dlite_swig_set_property(struct _DLiteInstance *inst, const char *name,
                             obj_t *obj);
bool dlite_instance_has_property(struct _DLiteInstance *inst, const char *name);



/* FIXME - how do we avoid duplicating these constants from dlite-schemas.h? */
#define BASIC_METADATA_SCHEMA  "http://onto-ns.com/meta/0.1/BasicMetadataSchema"
#define ENTITY_SCHEMA          "http://onto-ns.com/meta/0.3/EntitySchema"
#define COLLECTION_ENTITY      "http://onto-ns.com/meta/0.1/Collection"

/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-entity-python.i"
#endif
