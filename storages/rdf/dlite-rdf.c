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


/** Prefix and corrosponding IRI value for predicates */
#ifndef _P
#define _P "dm"                           /*!< prefix */
#endif
#ifndef _V
#define _V "http://emmo.info/datamodel#"  /*!< value */
#endif


/** Formatting flags

  The simple format serialise data and metadata differently, while

 */
typedef enum {
  fmtMetaAnnot=1,  /*!< Serialise metadata using the hasURI, hasDescription,
                        hasProperty and hasDescription properties. */
  fmtMetaVals=2    /*!< Serialise metadata as any other data using
                        hasDimensionValue and hasPropertyValue. */
} FmtFlags;


/** Storage for librdf backend. */
typedef struct {
  DLiteStorage_HEAD
  TripleStore *ts;    /*!< Pointer to triplestore. */
  char *store;        /*!< Name of storage. */
  char *base_uri;     /*!< Base uri to use in serialisation. */
  char *filename;     /*!< Name of optional input/output file. */
  char *format;       /*!< Format of optional input/output file. */
  char *mime_type;    /*!< Mime time of optional input/output file. */
  char *type_uri;     /*!< Type uri of optional input/output file. */
  FmtFlags fmtflags;  /*!< Formatting flags. */
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
  - meta-annot : bool
      Whether to serialise metadata using the hasURI, hasDescription,
      hasProperty and hasDescription properties.  Default: true
  - meta-vals : bool
      Whether to serialise metadata as any other data using
      hasDimensionValue and hasPropertyValue.  Default: false

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
  char *metaannot_descr = "Whether to serialise metadata using the hasURI, "
    "hasDescription, hasProperty and hasDescription properties.  Default: true";
  char *metavals_descr = "Whether to serialise metadata as any other data using"
    "hasDimensionValue and hasPropertyValue.  Default: false";
  DLiteOpt opts[] = {
    {'m', "mode",      "w",        mode_descr},
    {'s', "store",     "hashes",   store_descr},
    {'b', "base-uri",  NULL,       base_descr},
    {'f', "filename",  NULL,       filename_descr},
    {'F', "format",    "ntriples", format_descr},
    {'m', "mime-type", NULL,       mime_descr},
    {'t', "type-uri",  NULL,       type_descr},
    {'o', "options",   NULL,       options_descr},
    {'a', "meta-annot","yes",      metaannot_descr},
    {'v', "meta-vals", "no",       metavals_descr},
    {0, NULL, NULL, NULL}
  };
  char *optcopy = (options) ? strdup(options) : NULL;
  const char *mode, *opt;
  UNUSED(api);

  if (!(s = calloc(1, sizeof(RdfStorage)))) FAILCODE(dliteMemoryError, "allocation failure");

  /* parse options */
  if (dlite_option_parse(optcopy, opts, 1)) goto fail;
  mode = opts[0].value;
  s->store   =   (opts[1].value) ? strdup(opts[1].value) : NULL;
  s->base_uri =  (opts[2].value) ? strdup(opts[2].value) : NULL;
  s->filename =  (opts[3].value) ? strdup(opts[3].value) : NULL;
  s->format  =   (opts[4].value) ? strdup(opts[4].value) : NULL;
  s->mime_type = (opts[5].value) ? strdup(opts[5].value) : NULL;
  s->type_uri  = (opts[6].value) ? strdup(opts[6].value) : NULL;
  opt   = (opts[7].value) ? opts[7].value : NULL;
  s->fmtflags |= (atob(opts[8].value)) ? fmtMetaAnnot : 0;
  s->fmtflags |= (atob(opts[9].value)) ? fmtMetaVals : 0;

  s->flags |= dliteGeneric;
  if (strcmp(mode, "r") == 0 || strcmp(mode, "read") == 0) {
    s->flags |= dliteReadable;
    s->flags &= ~dliteWritable;
  } else if (strcmp(mode, "a") == 0 || strcmp(mode, "append") == 0) {
    s->flags |= dliteReadable;
    s->flags |= dliteWritable;
  } else if (strcmp(mode, "w") == 0 || strcmp(mode, "write") == 0) {
    s->flags &= ~dliteReadable;
    s->flags |= dliteWritable;
  } else {
    FAIL1("invalid \"mode\" value: '%s'. Must be \"w\" (writable) "
          "or \"r\" (read-only) ", mode);
  }
  if (!s->base_uri && (strcmp(s->store, "file") == 0 ||
                       strcmp(s->store, "hashes") == 0 ||
                       strcmp(s->store, "sqlite") == 0))
    s->base_uri = strdup(_P);

  /* if read-only, check that storage file exists for file-based storages */
  if (!(s->flags & dliteWritable)) {
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
  int retval = 0;

  if (s->flags & dliteWritable) {
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
      FILE *fp;
      if (s->base_uri)
        base_uri = librdf_new_uri(world, (unsigned char *)s->base_uri);
      if (s->type_uri)
        type_uri = librdf_new_uri(world, (unsigned char *)s->type_uri);

      buf = librdf_model_to_string(model, base_uri, s->format, s->mime_type,
                                   type_uri);

      if (strcmp(s->filename, "-") == 0) {
        fprintf(stdout, "%s", buf);
      } else if ((fp = fopen(s->filename, "w"))) {
        fprintf(fp, "%s", buf);
        fclose(fp);
      } else {
        retval = err(dliteIOError, "cannot write rdf file: %s", s->filename);
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
  return retval;
}


/* Returns pointer to object corresponding to subject `s` and predicate `p`
   or NULL on error.

   If `verbose` is non-zero, error messages will be printed. */
static const char *getobj(RdfStorage *rdf, const char *s, const char *p,
                          int verbose)
{
  TripleStore *ts = rdf->ts;
  const Triple *t;
  if (!(t = triplestore_find_first(ts, s, p, NULL, NULL))) {
    if (verbose) err(1, "missing s='%s' p='%s': %s", s, p, rdf->location);
    return NULL;
  }
  return t->o;
}

/* Returns number of triples matching (s, p, o) or -1 on error. */
static int count(TripleStore *ts, const char *s, const char *p, const char *o)
{
  TripleState state;
  int n=0;
  triplestore_init_state(ts, &state);
  while (triplestore_find(&state, s, p, o, NULL)) n++;
  triplestore_deinit_state(&state);
  return n;
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
  size_t i, *dims=NULL;
  int ok=0, n, j;
  char uuid[DLITE_UUID_LENGTH+1], muuid[DLITE_UUID_LENGTH+1];
  char *pid=NULL, *mid=NULL, *propiri=NULL;
  const char *value;

  errno = 0;

  /* find instance and metadata UUIDs */
  if (id) {
    dlite_get_uuid(uuid, id);
    pid = (s->base_uri) ? aprintf("%s:%s", s->base_uri, uuid) : NULL;
    if (!(value = triplestore_value(ts, pid, _P ":hasMeta", NULL, NULL,
                                  NULL, 0)))
      FAILCODE2(dliteLookupError,
                "cannot find instance '%s' in RDF storage: %s",
                pid, s->location);
    dlite_get_uuid(muuid, value);
  } else {
    triplestore_init_state(ts, &state);
    if ((t = triplestore_find(&state, NULL, _P ":hasMeta", NULL, NULL))) {
      pid = strdup(t->s);
      dlite_get_uuid(muuid, t->o);
    }
    t2 = triplestore_find(&state, NULL, _P ":hasMeta", NULL, NULL);
    triplestore_deinit_state(&state);
    if (!t) FAILCODE1(dliteLookupError,
                      "no instances in RDF storage: %s", s->location);

    if (t2) FAILCODE1(dliteLookupError, "ID must be provided if storage "
                      "holds more than one instance: %s", s->location);
    if (!(value = triplestore_value(ts, pid, _P ":hasUUID", NULL, NULL,
                                    NULL, 0)))
      FAILCODE2(dliteInconsistentDataError, "instance '%s' has no "
                _P ":hasUUID relation in RDF storage: %s", pid, s->location);
    dlite_get_uuid(uuid, value);
  }

  /* get/load metadata */
  mid = (s->base_uri) ? aprintf("%s:%s", s->base_uri, muuid) : NULL;
  if (!(meta = dlite_meta_get(muuid)) &&
      !(meta = dlite_meta_load(storage, muuid)))
    FAIL1("cannot load metadata: '%s'", t->o);

  /* allocate and read dimension values */
  if (meta->_ndimensions) {
    const char *name, *val;
    if (!(dims = calloc(meta->_ndimensions, sizeof(size_t))))
      FAILCODE(dliteMemoryError, "allocation failure");
    if (triplestore_find_first(ts, pid, _P ":hasDimensionValue", NULL, NULL)) {
      /* -- read dimension values */
      n = 0;
      triplestore_init_state(ts, &state);
      while ((t = triplestore_find(&state, pid, _P ":hasDimensionValue",
                                   NULL, NULL))) {
        char *dimval = strdup(t->o);
        if (!(name = getobj(s, dimval, _P ":hasLabel", 1))) goto fail;
        j = dlite_meta_get_dimension_index(meta, name);
        if (!(val = getobj(s, dimval, _P ":hasDimensionSize", 1))) goto fail;
        dims[j] = atoi(val);
        n++;
        free(dimval);
      }
      triplestore_deinit_state(&state);
      if (n != (int)meta->_ndimensions)
        FAIL4("entity %s expect %d dimension values, but got %d: %s",
              id, (int)meta->_ndimensions, n, s->location);
    } else if (strcmp(meta->uri, DLITE_ENTITY_SCHEMA) == 0) {
      /* -- infer dimension values */
      assert(meta->_ndimensions == 2);
      dims[0] = count(ts, pid, _P ":hasDimension", NULL);
      dims[1] = count(ts, pid, _P ":hasProperty", NULL);
    } else {
      FAIL2("missing dimension values for instance '%s' in storage '%s'",
            id, s->location);
    }
  }

  if (!(inst = dlite_instance_create(meta, dims, (id) ? id : uuid))) goto fail;
  if (!inst->uri && (t = triplestore_find_first(ts, pid, _P ":hasURI", NULL,
                                                NULL)))
    inst->uri = strdup(t->o);

  /* FIXME - should have been called by dlite_instance_create() */
  if (dlite_instance_is_meta(inst)) dlite_meta_init((DLiteMeta *)inst);

  n = 0;
  triplestore_init_state(ts, &state);
  while ((t = triplestore_find(&state, pid, _P ":hasPropertyValue", NULL,
                               NULL))) {
    /* -- read property values */
    DLiteProperty *p;
    const char *name, *val;
    size_t *pdims;
    void *ptr;
    char *prop=strdup(t->o);
    if (!(name = getobj(s, prop, _P ":hasLabel", 1))) goto fail;
    j = dlite_meta_get_property_index(meta, name);
    if (!(name = getobj(s, prop, _P ":hasLabel", 1))) goto fail;
    if ((j = dlite_meta_get_property_index(meta, name)) < 0) goto fail;
    if (!(val = getobj(s, prop, _P ":hasValue", 1))) goto fail;
    p = meta->_properties + j;
    pdims = DLITE_PROP_DIMS(inst, j);
    ptr = dlite_instance_get_property_by_index(inst, j);
    if (dlite_property_scan(val, ptr, p, pdims, dliteFlagRaw) < 0) goto fail;
    n++;
    free(prop);
  }
  triplestore_deinit_state(&state);

  /* Metadata is normally stored with dedicated relations according to the
     datamodel ontology. */
  if (n == 0 && strcmp(meta->uri, DLITE_ENTITY_SCHEMA) == 0) {
    const char *str;
    char **namep, **verp, **nsp, **descrp;
    DLiteDimension *d;
    DLiteProperty *p;

    /* -- read header: uri, description */
    namep = dlite_instance_get_property(inst, "name");
    verp = dlite_instance_get_property(inst, "version");
    nsp = dlite_instance_get_property(inst, "namespace");
    if (!(namep && verp && nsp))
      fatal(1, "%s should have name, version and namespace properties",
            DLITE_ENTITY_SCHEMA);
    if (!(str = getobj(s, pid, _P ":hasURI", 1))) goto fail;
    dlite_split_meta_uri(str, namep, verp, nsp);

    descrp = dlite_instance_get_property(inst, "description");
    if ((str = getobj(s, pid, _P ":hasDescription", 0)))
      *descrp = strdup(str);

    /* -- read dimensions */
    d = dlite_instance_get_property(inst, "dimensions");
    triplestore_init_state(ts, &state);
    while ((t = triplestore_find(&state, pid, _P ":hasDimension", NULL,
                                 NULL))) {
      propiri = strdup(t->o);
      if (!(str = getobj(s, propiri, _P ":hasLabel", 1))) goto fail;
      d->name = strdup(str);
      if ((str = getobj(s, propiri, _P ":hasDescription", 0)))
        d->description = strdup(str);
      d++;
      free(propiri);
      propiri = NULL;
    }
    triplestore_deinit_state(&state);


    /* -- read properties */
    p = dlite_instance_get_property(inst, "properties");
    triplestore_init_state(ts, &state);
    while ((t = triplestore_find(&state, pid, _P ":hasProperty", NULL,
                                 NULL))) {
      const char *name, *typename, *shape, *unit, *descr;

      /* save propiri so it is not overwritten by getobj() */
      propiri = strdup(t->o);

      if (!(name = getobj(s, propiri, _P ":hasLabel", 1))) goto fail;
      p->name = strdup(name);
      if (!(typename = getobj(s, propiri, _P ":hasType", 1))) goto fail;
      if (dlite_type_set_dtype_and_size(typename, &p->type, &p->size))
        goto fail;
      if ((unit = getobj(s, propiri, _P ":hasUnit", 0)))
        p->unit = strdup(unit);
      if ((descr = getobj(s, propiri, _P ":hasDescription", 0)))
        p->description = strdup(descr);

      /* count and allocate property dimensions */
      shape = getobj(s, propiri, _P ":hasFirstShape", 0);
      while (shape) {
        p->ndims++;
        shape = getobj(s, shape, _P ":hasNextShape", 0);
      }
      p->shape = calloc(p->ndims, sizeof(char *));

      /* assign property dimensions */
      i = 0;
      shape = getobj(s, propiri, _P ":hasFirstShape", 0);
      while (shape) {
        const char *expr;
        char buf[64];
        strncpy(buf, shape, sizeof(buf));
        if (!(expr = getobj(s, buf, _P ":hasDimensionExpression", 1)))
          FAIL2("%s has no dimension expression: %s", t2->s, s->location);
        p->shape[i++] = strdup(expr);
        shape = getobj(s, buf, _P ":hasNextShape", 0);
      }
      p++;
      n++;
      free(propiri);
      propiri = NULL;
    }
    triplestore_deinit_state(&state);

    /* reinitialise metadata after property dimensions have been set */
    dlite_meta_init((DLiteMeta *)inst);
  }
  if (n != (int)meta->_nproperties)
    FAIL4("entity %s expect %d property values, but got %d: %s",
          id, (int)meta->_nproperties, n, s->location);

  ok = 1;
 fail:
  if (pid) free(pid);
  if (mid) free(mid);
  if (propiri) free(propiri);
  if (dims) free(dims);
  if (!ok && inst) dlite_instance_decref(inst);
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
  if (!node) return err(1, "cannot create blank node: %s", id), NULL;
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
  DLiteMeta *meta = (dlite_instance_is_meta(inst)) ? (DLiteMeta *)inst : NULL;
  size_t i, bufsize=0, buf2size=0;
  int j, retval=1;
  char *buf=NULL, *buf2=NULL, *b1, *b2;
  triplestore_add_uri(ts, inst->uuid, "rdf:type", "owl:NamedIndividual");
  if (meta)
    triplestore_add_uri(ts, inst->uuid, "rdf:type", _P ":Entity");
  else
    triplestore_add_uri(ts, inst->uuid, "rdf:type", _P ":Object");
  triplestore_add(ts, inst->uuid, _P ":hasUUID", inst->uuid, "xsd:anyURI");
  triplestore_add(ts, inst->uuid, _P ":hasMeta", inst->meta->uri, NULL);
  if (inst->uri)
    triplestore_add(ts, inst->uuid, _P ":hasURI", inst->uri, NULL);

  /* Describe metadata with spesialised properties */
  if (meta && s->fmtflags & fmtMetaAnnot) {
    const char **descr = dlite_instance_get_property(inst, "description");
    if (descr)
      triplestore_add_en(ts, inst->uuid, _P ":hasDescription", *descr);

    for (i=0; i < meta->_ndimensions; i++) {
      DLiteDimension *d = meta->_dimensions + i;
      asnprintf(&buf, &bufsize, "%s/%s", inst->uuid, d->name);
      if (!(b1 = get_blank_node(ts, buf))) goto fail;
      triplestore_add_uri(ts, inst->uuid, _P ":hasDimension", b1);
      triplestore_add_uri(ts, b1, "rdf:type", _P ":Dimension");
      triplestore_add(ts, b1, _P ":hasLabel", d->name, "xsd:Name");
      triplestore_add_en(ts, b1, _P ":hasDescription", d->description);
      free(b1);
    }

    for (i=0; i < meta->_nproperties; i++) {
      DLiteProperty *p = meta->_properties + i;
      char typename[32];
      dlite_type_set_typename(p->type, p->size, typename, sizeof(typename));
      asnprintf(&buf, &bufsize, "%s/%s", inst->uuid, p->name);
      if (!(b1 = get_blank_node(ts, buf))) goto fail;
      asnprintf(&buf2, &buf2size, "%s/shape0", buf);
      if (!(b2 = get_blank_node(ts, buf2))) goto fail;
      triplestore_add_uri(ts, inst->uuid, _P ":hasProperty", b1);
      triplestore_add_uri(ts, b1, "rdf:type", _P ":Property");
      triplestore_add(ts, b1, _P ":hasLabel", p->name, "xsd:Name");
      triplestore_add(ts, b1, _P ":hasType", typename, "xsd:Name");
      if (p->ndims)
        triplestore_add_uri(ts, b1, _P ":hasFirstShape", b2);
      if (p->unit)
        triplestore_add(ts, b1, _P ":hasUnit", p->unit, "xsd:Name");
      if (p->description)
        triplestore_add_en(ts, b1, _P ":hasDescription", p->description);

      if (p->shape) {
        triplestore_add_uri(ts, b2, "rdf:type", _P ":Shape");
        triplestore_add(ts, b2, _P ":hasDimensionExpression", p->shape[0],
                        "xsd:string");
      }
      for (j=1; j < p->ndims; j++) {
        char *b;
        asnprintf(&buf2, &buf2size, "%s/shape%d", buf, j);
        if (!(b = get_blank_node(ts, buf2))) goto fail;
        triplestore_add_uri(ts, b2, _P ":hasNextShape", b);
        triplestore_add_uri(ts, b, "rdf:type", _P ":Shape");
        triplestore_add(ts, b, _P ":hasDimensionExpression", p->shape[j],
                        "xsd:string");
        free(b2);
        b2 = b;
      }
      free(b1);
      free(b2);
    }
  }

  if (!meta || s->fmtflags & fmtMetaVals) {
    /* Dimension values */
    for (i=0; i < inst->meta->_ndimensions; i++) {
      const char *name = inst->meta->_dimensions[i].name;
      asnprintf(&buf, &bufsize, "%s/dim_%s", inst->uuid, name);
      if (!(b1 = get_blank_node(ts, buf))) goto fail;
      asnprintf(&buf, &bufsize, "%d",
                (int)dlite_instance_get_dimension_size_by_index(inst, i));
      triplestore_add_uri(ts, inst->uuid, _P ":hasDimensionValue", b1);
      triplestore_add(ts, b1, _P ":hasLabel", name, "xsd:Name");
      triplestore_add(ts, b1, _P ":hasDimensionSize", buf, "xsd:integer");
      free(b1);
    }

    /* Property values */
    for (i=0; i < inst->meta->_nproperties; i++) {
      const DLiteProperty *p = dlite_meta_get_property_by_index(inst->meta, i);
      const void *ptr = dlite_instance_get_property_by_index(inst, i);
      const char *name = inst->meta->_properties[i].name;
      const size_t *shape = DLITE_PROP_DIMS(inst, i);
      asnprintf(&buf, &bufsize, "%s/val_%s", inst->uuid, name);
      if (!(b1 = get_blank_node(ts, buf))) goto fail;
      triplestore_add_uri(ts, inst->uuid, _P ":hasPropertyValue", b1);
      triplestore_add_uri(ts, b1, "rdf:type", "owl:NamedIndividual");
      triplestore_add_uri(ts, b1, "rdf:type", _P ":PropertyValue");
      triplestore_add(ts, b1, _P ":hasLabel", name, "xsd:Name");
      dlite_property_aprint(&buf, &bufsize, 0, ptr, p, shape, 0, -2,
                            dliteFlagRaw | dliteFlagStrip);
      triplestore_add(ts, b1, _P ":hasValue", buf, "rdf:PlainLiteral");
      free(b1);
    }
  }

  retval = 0;
 fail:
  if (buf) free(buf);
  if (buf2) free(buf2);
  return retval;
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
    return err(dliteMemoryError, "allocation failure"), NULL;
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
    t = triplestore_find(&riter->state, NULL, _P ":hasMeta", NULL, NULL);
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
  NULL,                                 /* flush */
  NULL,                                 /* help */

  /* queue api */
  rdf_iter_create,                      /* iterCreate */
  rdf_iter_next,                        /* iterNext */
  rdf_iter_free,                        /* iterFree */

  /* direct api */
  rdf_load_instance,                    /* loadInstance */
  rdf_save_instance,                    /* saveInstance */
  NULL,                                 /* deleteInstance */

  /* In-memory api */
  NULL,                                 /* memLoadInstance */
  NULL,                                 /* memSaveInstance */

  /* === API to deprecate === */
  NULL,                                 /* getUUIDs */

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
get_dlite_storage_plugin_api(void *state, int *iter)
{
  UNUSED(iter);
  dlite_globals_set(state);
  return &rdf_plugin;
}
