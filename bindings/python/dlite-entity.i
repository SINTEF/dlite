/* -*- C -*-  (not really, but good for syntax highlighting) */

<<<<<<< HEAD
=======
%{

/* Returns a pointer to a new target language object for property `i`. */
void *dlite_swig_get_property(const DLiteInstance *inst, const char *name)
{
  int i;
  if ((i = dlite_meta_get_property_index(inst->meta, name)) < 0) return NULL;
  return dlite_swig_get_property_by_index(inst, i);
}
%}

/* ----------------------------------------------------------------- */
/*  */
/* ----------------------------------------------------------------- */

>>>>>>> ALP-39-python_bindings
%numpy_typemaps(size_t, NPY_UINT, size_t)
%apply (int *IN_ARRAY1, int DIM1) { (int *dims, int ndims) };
%apply (size_t *IN_ARRAY1, size_t DIM1) { (size_t *dims, size_t ndims) };
%apply (size_t **ARGOUTVIEWM_ARRAY1, size_t *DIM1) {
  (size_t **dims, size_t *ndims) };


%rename(Instance) _DLiteInstance;
struct _DLiteInstance {
  %immutable;
  char uuid[DLITE_UUID_LENGTH+1];
  char *uri;
  int refcount;
  const struct _DLiteMeta *meta;
};




//%apply (size_t IN_ARRAY1, size_t DIM1) { (size_t *dims, size_t ndims) };
%extend _DLiteInstance {
  //_DLiteInstance(const char *metaid, size_t *dims, size_t ndims,
  //		 const char *id=NULL) {

  _DLiteInstance(const char *metaid, int *dims, int ndims,
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
  char * __repr__(void) {
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

  void save_url(const char *url) {
    dlite_instance_save_url(url, $self);
  }

  void _get_dimensions(size_t **dims, size_t *ndims) {
    *ndims = DLITE_NDIM($self);
    *dims = DLITE_DIMS($self);
    printf("*** _get_dimensions()\n");
    printf("    -> ndims=%zu\n", *ndims);
    printf("    -> dims=[");
    {
      size_t i;
      for (i=0; i<*ndims; i++) printf("%zu, ", *dims[i]);
      printf("]\n");
    }

  }
  %pythoncode %{
    dimensions = property(_get_dimensions, doc='Array of dimension sizes.')
  %}

};
