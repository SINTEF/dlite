/* -*- C -*-  (not really, but good for syntax highlighting) */

//%numpy_typemaps(size_t, NPY_UINT, size_t)


%rename(Instance) _DLiteInstance;
struct _DLiteInstance {
  char uuid[DLITE_UUID_LENGTH+1];
  char *uri;
  int refcount;
  const struct _DLiteMeta *meta;
};


//%apply (size_t IN_ARRAY1, size_t DIM1) { (size_t *dims, size_t ndims) };
%apply (int IN_ARRAY1, int DIM1) { (int *dims, int ndims) };
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
    DLiteInstance *inst;
    printf("*** inst\n");
    inst = dlite_instance_load_url(url);
    printf("*** inst=%p\n", (void *)inst);
    if (!inst) dlite_err(1, "XXX");
    printf("    inst=%p\n", (void *)inst);
    return inst;
  }
  ~_DLiteInstance() {
    dlite_instance_decref($self);
  }
  void save_url(const char *url) {
    dlite_instance_save_url(url, $self);
  }
};
