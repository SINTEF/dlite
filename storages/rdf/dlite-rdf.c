/* dlite-rdf.c -- DLite plugin for rdf */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <redland.h>

#include "config.h"

#include "boolean.h"
#include "utils/strtob.h"
#include "utils/err.h"

#include "triplestore.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-datamodel.h"


/** Storage for librdf backend. */
typedef struct {
  DLiteStorage_HEAD
  TripleStore *ts;
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
      - w: Writable (default)
      - r: Read-only
  - store : "hashes" | "memory" | "file" | "mysql" | "postgresql" | "sqlite" |
            "tstore" | "uri" | "virtuoso"
      Name of librdf storage module to use. The default is "hashes".
      See https://librdf.org/docs/api/redland-storage-modules.html
      for more info.
  - name : string
      Name of the storage.
  - options : string
      Comma-separated string of options to pass to the librdf storage module.

  Returns NULL on error.
*/
DLiteStorage *rdf_open(const DLiteStoragePlugin *api, const char *uri,
                       const char *options)
{
  RdfStorage *s=NULL;
  DLiteStorage *retval=NULL;
  char *mode_descr = "How to open storage.  Valid values are: "
    "\"w\" (writable, default); "
    "\"r\" (read-only)";
  char *store_descr = "librdf storage module.  One of: "
    "\"hashes\", \"memory\", \"file\", \"mysql\", \"postgresql\", \"sqlite\", "
    "\"tstore\", \"uri\" or \"virtuoso\".  See "
    "https://librdf.org/docs/api/redland-storage-modules.html";
  char *name_descr = "Name of the storage.";
  char *options_descr = "Comma-separated string of options to pass to "
    "the librdf storage module.";
  DLiteOpt opts[] = {
    {'m', "mode",    "w",       mode_descr},
    {'s', "store",   "hashes",  store_descr},
    {'n', "name",    "",        name_descr},
    {'o', "options", "",        options_descr},
    {0, NULL, NULL, NULL}
  };
  char *optcopy = (options) ? strdup(options) : NULL;
  const char *mode, *store, *name, *opt;
  UNUSED(api);
  UNUSED(uri);

  if (!(s = calloc(1, sizeof(RdfStorage)))) FAIL("allocation failure");

  /* parse options */
  if (dlite_option_parse(optcopy, opts, 1)) goto fail;
  mode  = opts[0].value;
  store = (opts[1].value) ? opts[1].value : NULL;
  name  = (opts[2].value) ? opts[2].value : NULL;
  opt   = (opts[3].value) ? opts[3].value : NULL;

  if (strcmp(mode, "r") == 0 || strcmp(mode, "read") == 0) {
    s->writable = 0;
  } else if (strcmp(mode, "w") == 0 || strcmp(mode, "write") == 0) {
    s->writable = 1;
  } else {
    FAIL1("invalid \"mode\" value: '%s'. Must be \"w\" (writable) "
          "or \"r\" (read-only) ", mode);
  }

  if (!(s->ts = triplestore_create_with_storage(store, name, opt))) goto fail;
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
  triplestore_free(s->ts);
  free(s);
  return 0;
}

/**
  Creates a new datamodel for rdf storage `storage`.

  If `id` exists in `storage`, the datamodel should describe the
  corresponding instance.

  Returns the new datamodel or NULL on error.
*/
DLiteDataModel *rdf_datamodel(const DLiteStorage *storage, const char *id)
{
  RdfDataModel *d=NULL;
  DLiteDataModel *retval=NULL;
  RdfStorage *s = (RdfStorage *)storage;
  UNUSED(id);
  UNUSED(s);
  if (!(d = calloc(1, sizeof(RdfDataModel))))
    FAIL("allocation failure");
  //d->api =
  //d->storage = storage;
  retval = (DLiteDataModel *)d;
 fail:
  if (!retval && d) free(d);
  return retval;
}




static DLiteStoragePlugin rdf_plugin = {
  "rdf",
  NULL,

  /* basic api */
  rdf_open,
  rdf_close,

  /* queue api */
  NULL,                            /* iterCreate */
  NULL,                            /* iterNext */
  NULL,                            /* iterFree */
  NULL, //dlite_rdf_get_uuids,

  /* direct api */
  NULL,
  NULL,

  /* datamodel api */
  rdf_datamodel,
  NULL, //dlite_rdf_datamodel_free,

  NULL, //dlite_rdf_get_metadata,
  NULL, //dlite_rdf_resolve_dimensions,
  NULL, //dlite_rdf_get_dimension_size,
  NULL, //dlite_rdf_get_property,

  /* -- datamodel api (optional) */
  NULL, //dlite_rdf_set_metadata,
  NULL, //dlite_rdf_set_dimension_size,
  NULL, //dlite_rdf_set_property,

  NULL, //dlite_rdf_has_dimension,
  NULL, //dlite_rdf_has_property,

  NULL, //dlite_rdf_get_dataname,
  NULL, //dlite_rdf_set_dataname,

  /* specialised api */
  //dlite_rdf_get_entity,
  //dlite_rdf_set_entity,

  /* internal data */
  NULL
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(int *iter)
{
  UNUSED(iter);
  return &rdf_plugin;
}
