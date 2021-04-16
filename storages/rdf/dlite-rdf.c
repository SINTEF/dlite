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


/**
  Stores instance `inst` to `storage`.  Returns non-zero on error.
 */
DLiteInstance *rdf_load_instance(const DLiteStorage *s, const char *uuid)
{
  UNUSED(s);
  UNUSED(uuid);
  return NULL;
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
    asnprintf(&buff, &buffsize, "%s#_dimval%d", inst->uuid, i);
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
    asnprintf(&buff, &buffsize, "%s#_propval%d", inst->uuid, i);
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



static DLiteStoragePlugin rdf_plugin = {
  "rdf",
  NULL,

  /* basic api */
  rdf_open,
  rdf_close,

  /* queue api */
  NULL,                                 /* iterCreate */
  NULL,                                 /* iterNext */
  NULL,                                 /* iterFree */
  NULL, //dlite_rdf_get_uuids,

  /* direct api */
  rdf_load_instance,                    /* loadInstance */
  rdf_save_instance,                    /* saveInstance */

  /* datamodel api */
  NULL, //rdf_datamodel,                /* dataModel */
  NULL, //dlite_rdf_datamodel_free,     /* dataModelFree */

  NULL, //dlite_rdf_get_metadata,       /* getMetaURI */
  NULL, //dlite_rdf_resolve_dimensions, /* resolveDimensions */
  NULL, //dlite_rdf_get_dimension_size, /* getDimensionSize */
  NULL, //dlite_rdf_get_property,       /* getProperty */

  /* -- datamodel api (optional) */
  NULL, //dlite_rdf_set_metadata,       /* setMetaURI */
  NULL, //dlite_rdf_set_dimension_size, /* setDimensionSize */
  NULL, //dlite_rdf_set_property,       /* setProperty */

  NULL, //dlite_rdf_has_dimension,      /* hasDimension */
  NULL, //dlite_rdf_has_property,       /* hasProperty */

  NULL, //dlite_rdf_get_dataname,       /* getDataName */
  NULL, //dlite_rdf_set_dataname,       /* setDataName */

  /* internal data */
  NULL                                  /* data */
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(int *iter)
{
  UNUSED(iter);
  return &rdf_plugin;
}
