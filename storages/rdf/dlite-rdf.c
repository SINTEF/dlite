/* dlite-rdf.c -- DLite plugin for rdf */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <redland.h>

#include "config.h"

#include "boolean.h"
#include "utils/compat.h"
#include "utils/strtob.h"
#include "utils/strutils.h"
#include "utils/globmatch.h"
#include "utils/err.h"

#include "triplestore.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-datamodel.h"


/* Prefix and corrosponding IRI value for predicates */
#ifndef _P
#define _P "dm"                           /* prefix */
#endif
#ifndef _V
#define _V "http://emmo.info/datamodel#"  /* value */
#endif


/** Storage for librdf backend. */
typedef struct {
  DLiteStorage_HEAD
  TripleStore *ts;  /*!< Pointer to triplestore. */
  char *store;      /*!< Name of storage. */
  char *base_uri;   /*!< Base uri to use in serialisation. */
  char *filename;   /*!< Name of optional input/output file. */
  char *format;     /*!< Format of optional input/output file. */
  char *mime_type;  /*!< Mime time of optional input/output file. */
  char *type_uri;   /*!< Type uri of optional input/output file. */
} RdfStorage;

/** Data model for librdf backend. */
typedef struct {
  DLiteDataModel_HEAD
} RdfDataModel;

/** */
typedef struct {
  TripleState state;
  char *pattern;
} RdfIter;



/**
  Opens `uri` and returns a newly created storage for it.

  The `api` argument can normally be ignored (it is needed for the
  Python storage backend).

  The `options` argument provies additional input to the driver.
  Which options that are supported varies between the plugins.  It
  should be a valid URL query string of the form:

      key1=value1;key2=value2...

  An ampersand (&) may be used instead of the semicolon (;).

  Valid `options` are:

  - mode : w | r
      Valid values are:
      - r: Read-only (default)
      - w: Writable, the store will be synced
  - store : "hashes" | "memory" | "file" | "mysql" | "postgresql" | "sqlite" |
            "tstore" | "uri" | "virtuoso"
      Name of librdf storage module to use. The default is "hashes".
      See https://librdf.org/docs/api/redland-storage-modules.html
      for more info.
  - base-uri : string
      Base uri to use in serialisation.
  - filename : string
      Name of optional input/output file.
  - format : "atom" | "json" | "ntriples" | "rdfxml" | "rdfxml-abbrev" |
             "rdfxml-xmp" | "turtle" | "rss-1.0" | "dot"
      Format of optional input/output file. See also
      https://librdf.org/raptor/api-1.4/raptor-serializers.html
  - mime-type : string
      Mime type for format of optional input/output file.
  - type-uri : string
      Uri specifying format of optional input/output file.
  - options : string
      Comma-separated string of options to pass to the librdf storage module.

  Returns NULL on error.
*/
DLiteStorage *rdf_open(const DLiteStoragePlugin *api, const char *uri,
                       const char *options)
{
  RdfStorage *s=NULL;
  DLiteStorage *retval=NULL;
  char *mode_descr =
    "How to open storage.  Valid values are: "
    "\"w\" (writable, default); "
    "\"r\" (read-only)";
  char *store_descr =
    "librdf storage module.  One of: "
    "\"hashes\", \"memory\", \"file\", \"mysql\", \"postgresql\", \"sqlite\", "
    "\"tstore\", \"uri\" or \"virtuoso\".  See also "
    "https://librdf.org/docs/api/redland-storage-modules.html";
  char *base_descr =
    "Base URI to use in serialisation.";
  char *filename_descr =
    "Name of optional input/output file.";
  char *format_descr =
    "Format of optional input/output file.  One of: "
    "\"atom\", \"json\", \"ntriples\", \"rdfxml\", \"rdfxml-abbrev\", "
    "\"rdfxml-xmp\", \"turtle\", \"rss-1.0\" or \"dot\"  See also "
    "https://librdf.org/raptor/api-1.4/raptor-serializers.html";
  char *mime_descr =
    "Mime type for format of optional input/output file.";
  char *type_descr =
    "Uri specifying format of optional input/output file.";
  char *options_descr =
    "Comma-separated string of options to pass to the librdf storage module.";
  DLiteOpt opts[] = {
    {'m', "mode",      "w",        mode_descr},
    {'s', "store",     "hashes",   store_descr},
    {'b', "base-uri",  NULL,       base_descr},
    {'f', "filename",  NULL,       filename_descr},
    {'F', "format",    "ntriples", format_descr},
    {'m', "mime-type", NULL,       mime_descr},
    {'t', "type-uri",  NULL,       type_descr},
    {'o', "options",   NULL,       options_descr},
    {0, NULL, NULL, NULL}
  };
  char *optcopy = (options) ? strdup(options) : NULL;
  const char *mode, *opt;
  UNUSED(api);

  if (!(s = calloc(1, sizeof(RdfStorage)))) FAIL("allocation failure");

  /* parse options */
  if (dlite_option_parse(optcopy, opts, 1)) goto fail;
  mode  = opts[0].value;
  s->store   =   (opts[1].value) ? strdup(opts[1].value) : NULL;
  s->base_uri =  (opts[2].value) ? strdup(opts[2].value) : NULL;
  s->filename =  (opts[3].value) ? strdup(opts[3].value) : NULL;
  s->format  =   (opts[4].value) ? strdup(opts[4].value) : NULL;
  s->mime_type = (opts[5].value) ? strdup(opts[5].value) : NULL;
  s->type_uri  = (opts[6].value) ? strdup(opts[6].value) : NULL;
  opt   = (opts[7].value) ? opts[7].value : NULL;

  if (strcmp(mode, "r") == 0 || strcmp(mode, "read") == 0) {
    s->writable = 0;
  } else if (strcmp(mode, "w") == 0 || strcmp(mode, "write") == 0) {
    s->writable = 1;
  } else {
    FAIL1("invalid \"mode\" value: '%s'. Must be \"w\" (writable) "
          "or \"r\" (read-only) ", mode);
  }
  if (!s->base_uri && (strcmp(s->store, "file") == 0 ||
                       strcmp(s->store, "hashes") == 0 ||
                       strcmp(s->store, "sqlite") == 0))
    s->base_uri = strdup(_P);

  /* if read-only, check that storage file exists for file-based storages */
  if (!s->writable) {
    if (strcmp(s->store, "file") == 0) {
      FILE *fp;
      if (!(fp = fopen(uri, "r"))) FAIL1("cannot open storage: %s", uri);
      fclose(fp);
    }
  }

  /* create triplestore */
  if (!(s->ts = triplestore_create_with_storage(s->store, uri, opt))) goto fail;
  triplestore_set_namespace(s->ts, s->base_uri);

  retval = (DLiteStorage *)s;
 fail:
  if (optcopy) free(optcopy);
  if (!retval && s) {
    if (s->ts) triplestore_free(s->ts);
    free(s);
  }
  return retval;
}


/**
  Closes data handle json. Returns non-zero on error.
 */
int rdf_close(DLiteStorage *storage)
{
  RdfStorage *s = (RdfStorage *)storage;

  if (s->writable) {
    librdf_world *world = triplestore_get_world(s->ts);
    librdf_model *model = triplestore_get_model(s->ts);
    assert(world);
    assert(model);

    /* Sync storage */
    librdf_model_sync(model);

    /* Store to file */
    // FIXME - send directly to a serializer instead of writing to string...
    if (s->filename) {
      unsigned char *buf;
      librdf_uri *base_uri=NULL, *type_uri=NULL;
      if (s->base_uri)
        base_uri = librdf_new_uri(world, (unsigned char *)s->base_uri);
      if (s->type_uri)
        type_uri = librdf_new_uri(world, (unsigned char *)s->type_uri);

      buf = librdf_model_to_string(model, base_uri, s->format, s->mime_type,
                                   type_uri);

      if (strcmp(s->filename, "-") == 0) {
        fprintf(stdout, "%s", buf);
      } else {
        FILE *fp = fopen(s->filename, "w");
        fprintf(fp, "%s", buf);
        fclose(fp);
      }

      if (base_uri) librdf_free_uri(base_uri);
      if (type_uri) librdf_free_uri(type_uri);
      free(buf);
    }
  }

  triplestore_free(s->ts);
  if (s->store) free(s->store);
  if (s->base_uri) free(s->base_uri);
  if (s->filename) free(s->filename);
  if (s->format) free(s->format);
  if (s->mime_type) free(s->mime_type);
  if (s->type_uri) free(s->type_uri);
  return 0;
}


/* Returns pointer to object corresponding to subject `s` and predicate `p`
   or NULL on error. */
static const char *getobj(RdfStorage *rdf, const char *s, const char *p)
{
  TripleStore *ts = rdf->ts;
  const Triple *t;
  if (!(t = triplestore_find_first(ts, s, p, NULL)))
    return err(1, "missing s='%s' p='%s': %s", s, p, rdf->location), NULL;
  return t->o;
}


/*
  Loads instance from storage `s`.  Returns non-zero on error.
 */
DLiteInstance *rdf_load_instance(const DLiteStorage *storage, const char *id)
{
  RdfStorage *s = (RdfStorage *)storage;
  TripleStore *ts = s->ts;
  TripleState state;
  const Triple *t=NULL, *t2;
  DLiteInstance *inst=NULL;
  DLiteMeta *meta;
  size_t i, size=0, *dims=NULL;
  int ok=0;
  char uuid[DLITE_UUID_LENGTH+1], muuid[DLITE_UUID_LENGTH+1];
  char *pid=NULL, *mid=NULL, *buf=NULL;

  errno = 0;
  dlite_get_uuid(uuid, id);
  pid = (s->base_uri) ? aprintf("%s:%s", s->base_uri, uuid) : NULL;

  triplestore_init_state(ts, &state);
  while ((t2 = triplestore_find(&state, pid, _P ":hasMeta", NULL))) {
    if (t) FAIL1("UUID must be provided if storage holds "
                 "more than one instance: %s", s->location);
    t = t2;
  }
  if (!t) FAIL2("no instance with UUID %s in store: %s", pid, s->location);

  /* get/load metadata */
  dlite_get_uuid(muuid, t->o);
  mid = (s->base_uri) ? aprintf("%s:%s", s->base_uri, muuid) : NULL;
  if (!(meta = dlite_meta_get(muuid)) &&
      !(meta = dlite_meta_load(storage, muuid)))
    FAIL1("cannot load metadata: '%s'", t->o);

  /* allocate and read dimension values */
  if (meta->_ndimensions) {
    const char *dim, *val;
    if (!(dims = calloc(meta->_ndimensions, sizeof(size_t))))
      FAIL("allocation failure");
    /* read first dimension value */
    if (!(dim = getobj(s, pid, _P ":hasFirstDimensionValue"))) goto fail;
    if (strput(&buf, &size, 0, dim) < 0) FAIL("allocation failure");
    if (!(val = getobj(s, buf, _P ":hasIntegerValue"))) goto fail;
    dims[0] = strtol(val, NULL, 0);
    /* read remaining dimension values */
    for (i=1; i<meta->_ndimensions; i++) {
      if (!(dim = getobj(s, buf, _P ":hasNextElement"))) goto fail;
      if (strput(&buf, &size, 0, dim) < 0) FAIL("allocation failure");
      if (!(val = getobj(s, buf, _P ":hasIntegerValue"))) goto fail;
      dims[i] = strtol(t->o, NULL, 0);
    }
  }

  if (!(inst = dlite_instance_create(meta, dims, (id) ? id : uuid))) goto fail;

  /* read first property value */
  if (meta->_nproperties) {
    const char *prop, *val;
    void *ptr;
    DLiteProperty *p = meta->_properties;
    size_t *pdims = DLITE_PROP_DIMS(inst, 0);
    if (!(prop = getobj(s, pid, _P ":hasFirstPropertyValue"))) goto fail;
    if (strput(&buf, &size, 0, prop) < 0) FAIL("allocation failure");
    if (!(val = getobj(s, buf, _P ":hasValue"))) goto fail;
    ptr = dlite_instance_get_property_by_index(inst, 0);
    if (dlite_property_scan(val, ptr, p, pdims, 0) < 0) goto fail;

    /* read remaining property values */
    for (i=1; i < meta->_nproperties; i++) {
      p = meta->_properties + i;
      pdims = DLITE_PROP_DIMS(inst, i);
      if (!(prop = getobj(s, buf, _P ":hasNextElement"))) goto fail;
      if (strput(&buf, &size, 0, prop) < 0) FAIL("allocation failure");
      if (!(val = getobj(s, buf, _P ":hasValue"))) goto fail;
      ptr = dlite_instance_get_property_by_index(inst, i);
      if (dlite_property_scan(t->o, ptr, p, pdims, 0) < 0) goto fail;
    }
  }

  if (!inst->uri && (t = triplestore_find_first(ts, pid, _P ":hasURI", NULL)))
    inst->uri = strdup(t->o);

  ok = 1;
 fail:
  if (pid) free(pid);
  if (mid) free(mid);
  if (buf) free(buf);
  if (dims) free(dims);
  if (!ok && inst) dlite_instance_decref(inst);
  triplestore_deinit_state(&state);
  return (ok) ? inst : NULL;
}

/**
  Returns a UTF-8 encoded string for a new blank node, based on `id`.
  If `id` is NULL, an internally generated node is created.
  Returns NULL on error.
 */
static char *get_blank_node(TripleStore *ts, const char *id)
{
  char *str;
  librdf_world *world = triplestore_get_world(ts);
  librdf_node *node =
    librdf_new_node_from_blank_identifier(world, (unsigned char *)id);
  if (!node) return NULL;
  str = (char *)librdf_node_get_blank_identifier(node);
  if (str) str = strdup(str);
  librdf_free_node(node);
  return str;
}


/**
  Stores instance `inst` to `storage`.  Returns non-zero on error.
 */
int rdf_save_instance(DLiteStorage *storage, const DLiteInstance *inst)
{
  RdfStorage *s = (RdfStorage *)storage;
  TripleStore *ts = s->ts;
  size_t i, buffsize=0;
  char *buff=NULL, *b1=NULL, *b2=NULL;
  if (dlite_instance_is_data(inst))
    triplestore_add_uri(ts, inst->uuid, "rdf:type", _P ":Object");
  else
    triplestore_add_uri(ts, inst->uuid, "rdf:type", _P ":Entity");
  if (inst->uri)
    triplestore_add_uri(ts, inst->uuid, _P ":hasURI", inst->uri);
  triplestore_add_uri(ts, inst->uuid, _P ":hasMeta", inst->meta->uri);

  /* Dimension values */
  if (inst->meta->_ndimensions) {
    asnprintf(&buff, &buffsize, "%s#_dimval0", inst->uuid);
    b1 = get_blank_node(ts, buff);
    triplestore_add_uri(ts, b1, "rdf:type", _P ":DimensionValue");
    triplestore_add_uri(ts, inst->uuid, _P ":hasFirstDimensionValue", b1);
    asnprintf(&buff, &buffsize, "%zu",
             dlite_instance_get_dimension_size_by_index(inst, 0));
    triplestore_add2(ts, b1, _P ":hasIntegerValue", buff,
                     1, NULL, "xsd:integer");
  }
  for (i=1; i < inst->meta->_ndimensions; i++) {
    asnprintf(&buff, &buffsize, "%s#_dimval%zu", inst->uuid, i);
    b2 = get_blank_node(ts, buff);
    triplestore_add_uri(ts, b2, "rdf:type", _P ":DimensionValue");
    triplestore_add_uri(ts, b1, _P ":hasNextElement", b2);
    asnprintf(&buff, &buffsize, "%zu",
             dlite_instance_get_dimension_size_by_index(inst, i));
    triplestore_add2(ts, b2, _P ":hasIntegerValue", buff,
                     1, NULL, "xsd:integer");
    free(b1);
    b1 = b2;
  }
  if (b1 && !b2) free(b1);
  if (b2) free(b2);

  /* Property values */
  if (inst->meta->_nproperties) {
    const DLiteProperty *p = dlite_meta_get_property_by_index(inst->meta, 0);
    const void *ptr = dlite_instance_get_property_by_index(inst, 0);
    asnprintf(&buff, &buffsize, "%s#_propval0", inst->uuid);
    b1 = get_blank_node(ts, buff);
    triplestore_add_uri(ts, b1, "rdf:type", _P ":PropertyValue");
    triplestore_add_uri(ts, inst->uuid, _P ":hasFirstPropertyValue", b1);
    dlite_type_aprint(&buff, &buffsize, 0, ptr, p->type, p->size, 0, -2, 0);
    triplestore_add2(ts, b1, _P ":hasValue", buff, 1, NULL, "rdf:PlainLiteral");
  }
  for (i=1; i < inst->meta->_nproperties; i++) {
    const DLiteProperty *p = dlite_meta_get_property_by_index(inst->meta, i);
    const void *ptr = dlite_instance_get_property_by_index(inst, i);
    asnprintf(&buff, &buffsize, "%s#_propval%zu", inst->uuid, i);
    b2 = get_blank_node(ts, buff);
    triplestore_add_uri(ts, b2, "rdf:type", _P ":PropertyValue");
    triplestore_add_uri(ts, b1, _P ":hasNextElement", b2);
    dlite_property_aprint(&buff, &buffsize, 0, ptr, p,
                          DLITE_PROP_DIMS(inst, i), 0, -2, 0);
    triplestore_add2(ts, b2, _P ":hasValue", buff, 1, NULL, "rdf:PlainLiteral");
    free(b1);
    b1 = b2;
  }
  free(buff);
  if (b1 && !b2) free(b1);
  if (b2) free(b2);

  return 0;
}


/*
  Returns a new iterator over all instances in storage `s` who's metadata
  URI matches `pattern`.

  Returns NULL on error.
 */
void *rdf_iter_create(const DLiteStorage *storage, const char *pattern)
{
  RdfStorage *s = (RdfStorage *)storage;
  TripleStore *ts = s->ts;
  RdfIter *iter;
  if (!(iter = calloc(1, sizeof(RdfIter))))
    return err(1, "allocation failure"), NULL;
  iter->pattern = (pattern) ? strdup(pattern) : NULL;
  triplestore_init_state(ts, &iter->state);
  return iter;
}

/*
  Free's iterator created with IterCreate().
 */
void rdf_iter_free(void *iter)
{
  RdfIter *riter = iter;
  if (riter->pattern) free(riter->pattern);
  triplestore_deinit_state(&riter->state);
  free(iter);
}

/*
  Writes the UUID to buffer pointed to by `buf` of the next instance
  in `iter`, where `iter` is an iterator created with IterCreate().

  Returns zero on success, 1 if there are no more UUIDs to iterate
  over and a negative number on other errors.
 */
int rdf_iter_next(void *iter, char *buf)
{
  RdfIter *riter = iter;
  const Triple *t;
  while (1) {
    t = triplestore_find(&riter->state, NULL, _P ":hasMeta", NULL);
    if (!t) return 1;
    if (!riter->pattern) break;
    if (globmatch(riter->pattern, t->o) == 0) break;
  }
  assert(t);
  if (dlite_get_uuid(buf, t->o) < 0)
    return err(-1, "cannot create uuid from '%s'", t->o);
  return 0;
}




static DLiteStoragePlugin rdf_plugin = {
  "rdf",                                /* name */
  NULL,                                 /* freeapi */

  /* basic api */
  rdf_open,                             /* open */
  rdf_close,                            /* close */

  /* queue api */
  rdf_iter_create,                      /* iterCreate */
  rdf_iter_next,                        /* iterNext */
  rdf_iter_free,                        /* iterFree */
  NULL,                                 /* getUUIDs */

  /* direct api */
  rdf_load_instance,                    /* loadInstance */
  rdf_save_instance,                    /* saveInstance */

  /* datamodel api */
  NULL,                                 /* dataModel */
  NULL,                                 /* dataModelFree */

  NULL,                                 /* getMetaURI */
  NULL,                                 /* resolveDimensions */
  NULL,                                 /* getDimensionSize */
  NULL,                                 /* getProperty */

  /* -- datamodel api (optional) */
  NULL,                                 /* setMetaURI */
  NULL,                                 /* setDimensionSize */
  NULL,                                 /* setProperty */

  NULL,                                 /* hasDimension */
  NULL,                                 /* hasProperty */

  NULL,                                 /* getDataName */
  NULL,                                 /* setDataName */

  /* internal data */
  NULL                                  /* data */
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(int *iter)
{
  UNUSED(iter);
  return &rdf_plugin;
}
