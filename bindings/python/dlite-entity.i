/* -*- C -*-  (not really, but good for syntax highlighting) */


%rename(Instance) _DLiteInstance;

struct _DLiteInstance {
  char uuid[DLITE_UUID_LENGTH+1];
  char *uri;
  int refcount;
  const struct _DLiteMeta *meta;
};

%extend _DLiteInstance {
  _DLiteInstance(const char *metaid, const size_t *dims, const char *id) {
    return dlite_instance_create_from_id(metaid, dims, id);
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
