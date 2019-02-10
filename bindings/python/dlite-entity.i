/* -*- C -*-  (not really, but good for syntax highlighting) */

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
    free($self);
  }

  %pythoncode %{
    def __repr__(self): return 'Dimension(name=%r, description=%r)' % (
        self.name, self.description)
  %}

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
      if (!(p->dims = dlite_swig_copy_array(1, &p->ndims, type, size, dims))) {
        free(p->name);
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
    free($self);
  }

  obj_t *get_dims(void) {
    obj_t *arr;
    int *dims = malloc($self->ndims*sizeof(int));
    memcpy(dims, $self->dims, $self->ndims*sizeof(int));
    if (!(arr = dlite_swig_get_array(NULL, $self->ndims, dims,
                                     dliteInt, sizeof(int), dims)))
      free(dims);
    return arr;
  }
  /*
  obj_t set_dims(obj_t *arr) {
  }
  */
  %pythoncode %{
    def __repr__(self): return 'Property(name=%r,\n        type=%d,\n        size=%d,\n        ndims=%d,\n        dims=%r,\n        unit=%r,\n        description=%r)' % (self.name, self.type, self.size, self.ndims, self.dims if self.ndims else None, self.unit, self.description)

    dims = property(get_dims, doc='Array of dimension indices.')
  %}
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
  const struct _DLiteMeta *meta;
};


%extend _DLiteInstance {

  _DLiteInstance(const char *metaid, int ndims, int *dims,
		 const char *id=NULL) {
    DLiteInstance *inst;
    DLiteMeta *meta;
    size_t i, *d, n=ndims;
    if (!(meta = dlite_metastore_get(metaid)))
      return dlite_err(1, "cannot find metadata '%s'", metaid), NULL;
    if (n != meta->ndimensions)
      return dlite_err(1, "%s has %zu dimensions",
		       metaid, meta->ndimensions), NULL;
    d = malloc(n * sizeof(size_t));
    for (i=0; i<n; i++) d[i] = dims[i];
    inst = dlite_instance_create(meta, d, id);
    free(d);
    return inst;
  }

  _DLiteInstance(const char *url) {
    DLiteInstance *inst = dlite_instance_load_url(url);
    if (inst) dlite_errclr();
    return inst;
  }

  ~_DLiteInstance() {
    dlite_instance_decref($self);
  }

  %newobject __repr__;
  char *__repr__(void) {
    int n=0;
    char buff[64];
    n += snprintf(buff+n, sizeof(buff)-n, "<Instance:");
    if ($self->uri && $self->uri[0])
      n += snprintf(buff+n, sizeof(buff)-n, " uri='%s'", $self->uri);
    else
      n += snprintf(buff+n, sizeof(buff)-n, " uuid='%s'", $self->uuid);
    n += snprintf(buff+n, sizeof(buff)-n, ">");
    return strdup(buff);
  }

  int __len__(void) {
    return $self->meta->nproperties;
  }

  void save_url(const char *url) {
    dlite_instance_save_url(url, $self);
  }

  %newobject __get_dimensions__;
  obj_t *get_dimensions() {
    int dims[1] = { DLITE_NDIM($self) };
    return dlite_swig_get_array($self, 1, dims, dliteUInt, sizeof(size_t),
                                DLITE_DIMS($self));
  }
  %pythoncode %{
    dimensions = property(get_dimensions, doc='Array of dimension sizes.')
  %}

  %newobject __get_property__;
  obj_t *get_property(const char *name) {
    return dlite_swig_get_property($self, name);
  }
  obj_t *get_property(int i) {
    return dlite_swig_get_property_by_index($self, i);
  }
  %pythoncode %{
    def __getitem__(self, ind): return self.get_property(ind)
  %}

  void set_property(const char *name, obj_t *obj) {
    dlite_swig_set_property($self, name, obj);
  }
  void set_property(int i, obj_t *obj) {
    dlite_swig_set_property_by_index($self, i, obj);
  }
  %pythoncode %{
    def __setitem__(self, ind, val): self.set_property(ind, val)
  %}


};
