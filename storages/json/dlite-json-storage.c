/* dlite-json-storage.c -- DLite plugin for json */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "config.h"

#include "utils/err.h"
#include "utils/strtob.h"
#include "utils/jstore.h"
#include "utils/map.h"
#include "dlite.h"
#include "dlite-storage-plugins.h"
#include "dlite-macros.h"

typedef struct { char uuid[DLITE_UUID_LENGTH+1]; } uuid_t;
typedef map_t(uuid_t) map_uuid_t;

/** Storage for json backend. */
typedef struct {
  DLiteStorage_HEAD
  JStore *jstore;       /* json storage */
  DLiteJsonFlag flags;  /* output flags */
  int changed;          /* whether the storage is changed */
  map_uuid_t ids;       /* maps uuids to ids */
} DLiteJsonStorage;


/** Returns default mode:
    - 'w': if `uri` if we can't open uri
    - 'a': if `uri` is in data format
    - 'r': otherwise
*/
static int default_mode(const char *uri)
{
  int mode;
  JStore *js = jstore_open();
  if (jstore_update_from_file(js, uri))
    mode = 'w';
  else
    mode = (jstore_get(js, "properties")) ? 'r' : 'a';
  jstore_close(js);
  return mode;
}


/**
  Returns an url to the metadata.

  Valid `options` are:

  - mode : r | w | a
      Valid values are:
      - r   Open existing file for read-only
      - w   Truncate existing file or create new file
      - a   Append to existing file or create new file (default)
  - with-uuid : yes | no
      Whether to write output in compact format.
  - as-data : yes | no
  - meta : yes | no (deprecated)
      Whether to format output as metadata. Alias for `with-uuid`
  - compact : yes | no (deprecated)
      Whether to write output in compact format. Alias for `as-data`
  - useid: translate | require | keep (deprecated)
      How to use the ID.
 */
DLiteStorage *json_open(const DLiteStoragePlugin *api, const char *uri,
                        const char *options)
{
  DLiteJsonStorage *s;
  DLiteStorage *retval=NULL;
  char *mode_descr = "How to open storage.  Valid values are: "
    "\"r\" (read-only); "
    "\"w\" (truncate existing storage or create a new one); "
    "\"a\" (appends to existing storage or creates a new one)";
  DLiteOpt opts[] = {
    {'m', "mode",      "", mode_descr},
    {'u', "with-uuid", "false", "Whether to include uuid in output"},
    {'d', "as-data",   "false", "Whether to write metadata as data"},
    {'c', "compact",   "false", "Aliad for `as-data=false` (deprecated)"},
    {'M', "meta",      "false", "Alias for `with-uuid` (deprecated)"},
    {'U', "useid",     "",      "Unused (deprecated)"},
    {0, NULL, NULL, NULL}
  };
  int load;  // whether to load uri

  /* parse options */
  char *optcopy = (options) ? strdup(options) : NULL;
  if (dlite_option_parse(optcopy, opts, 1)) goto fail;

  char mode = *opts[0].value;
  int withuuid = atob(opts[1].value);
  int asdata = atob(opts[2].value);

  if (!(s = calloc(1, sizeof(DLiteJsonStorage)))) FAIL("allocation failure");
  s->api = api;

  if (!(s->jstore = jstore_open())) goto fail;

  if (!mode) mode = default_mode(uri);
  switch (mode) {
  case 'r':
    load = 1;
    s->writable = 0;
    break;
  case 'a':
    load = 1;
    s->writable = 1;
    break;
  case 'w':
    load = 0;
    s->writable = 1;
    break;
  default:
    FAIL1("invalid \"mode\" value: '%c'. Must be \"r\" (read-only), "
          "\"w\" (write) or \"a\" (append)", mode);
  }

  if (load) {
    DLiteJsonFormat fmt = dlite_jstore_loadf(s->jstore, uri);
    if (fmt < 0) goto fail;
    if (fmt == dliteJsonMetaFormat) s->writable = 0;
    dlite_storage_paths_append(uri);
  }

  if (withuuid < 0) FAIL1("invalid boolean value for `with-uuid=%s`.",
                          opts[1].value);
  if (asdata < 0) FAIL1("invalid boolean value for `as-data=%s`.",
                        opts[2].value);
  if (atob(opts[3].value) > 0) asdata = 0;
  if (atob(opts[4].value) > 0) withuuid = 1;

  if (withuuid) s->flags |= dliteJsonWithUuid;
  if (asdata) s->flags |= dliteJsonMetaAsData;

  retval = (DLiteStorage *)s;

 fail:
  if (optcopy) free(optcopy);
  if (!retval && s) dlite_storage_close((DLiteStorage *)s);

  return retval;
}


/**
  Closes data handle json. Returns non-zero on error.
 */
int json_close(DLiteStorage *s)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  int stat=0;
  if (js->writable && js->changed)
    stat = jstore_to_file(js->jstore, js->location);
  stat |= jstore_close(js->jstore);
  return stat;
}


/**
  Load instance `id` from storage `s` and return it.
  NULL is returned on error.
 */
DLiteInstance *json_load(const DLiteStorage *s, const char *id)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  const char *buf=NULL;
  char uuid[DLITE_UUID_LENGTH+1];

  if (!id || !*id) {
    JStoreIter iter;
    if (jstore_iter_init(js->jstore, &iter)) goto fail;
    if (!(id = jstore_iter_next(&iter)))
      FAIL1("cannot load instance from empty storage \"%s\"", s->location);
    if (jstore_iter_next(&iter)) {
      FAIL1("id is required when loading from storage with more "
            "than one instance: %s", s->location);
    }
    if (jstore_iter_deinit(&iter)) goto fail;
  } else if (dlite_get_uuid(uuid, id) == 5) {
    buf = jstore_get(js->jstore, uuid);
  }
  if (!buf && !(buf = jstore_get(js->jstore, id)))
    FAIL2("no instance with id \"%s\" in storage \"%s\"", id, s->location);
  return dlite_json_sscan(buf, id, NULL);
 fail:
  return NULL;
}


/**
  Saves instance `inst` to storage `s`.  Returns non-zero on error.
*/
int json_save(DLiteStorage *s, const DLiteInstance *inst)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  if (!s->writable)
    return errx(1, "storage \"%s\" is not writable", s->location);
  if (dlite_jstore_add(js->jstore, inst, js->flags)) return 1;
  js->changed = 1;
  return 0;
}


/**
  Creates and returns a new iterator used by dlite_json_iter_next().

  If `metaid` is not NULL, dlite_json_iter_next() will only iterate over
  instances whos metadata corresponds to this id.

  Returns new iterator or NULL on error.
 */
void *json_iter_create(const DLiteStorage *s, const char *metaid)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  return dlite_jstore_iter_create(js->jstore, metaid);
}

/**
  Writes the uuid of the next instance to `buf`, where `iter` is an
  iterator returned by dlite_json_iter_create().

  Returns zero on success, 1 if there are no more UUIDs to iterate
  over and a negative number on other errors.
 */
int json_iter_next(void *iter, char *buf)
{
  const char *id;
  if (!(id = dlite_jstore_iter_next(iter))) return 1;
  if (dlite_get_uuid(buf, id) < 0) return -1;
  return 0;
}

/**
  Free's iterator created with dlite_json_iter_create().
 */
void json_iter_free(void *iter)
{
  //dlite_json_iter_free(iter);
  dlite_jstore_iter_free(iter);
}


static DLiteStoragePlugin dlite_json_plugin = {
  /* head */
  "json",                   /* name */
  NULL,                     /* freeapi */

  /* basic api */
  json_open,                /* open */
  json_close,               /* close */

  /* queue api */
  json_iter_create,         /* iterCreate */
  json_iter_next,           /* iterNext */
  json_iter_free,           /* iterFree */
  NULL,                     /* getUUIDs */

  /* direct api */
  json_load,                /* loadInstance */
  json_save,                /* saveInstance */

  /* datamodel api */
  NULL,                     /* dataModel */
  NULL,                     /* dataModelFree */

  NULL,                     /* getMetaURI */
  NULL,                     /* resolveDimensions */
  NULL,                     /* getDimensionSize */
  NULL,                     /* getProperty */

  /* -- datamodel api (optional) */
  NULL,                     /* setMetaURI */
  NULL,                     /* setDimensionSize */
  NULL,                     /* setProperty */

  NULL,                     /* hasDimension */
  NULL,                     /* hasProperty */

  NULL,                     /* getDataName, obsolute */
  NULL,                     /* setDataName, obsolute */

  /* internal data */
  NULL                      /* data */
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(void *state, int *iter)
{
  UNUSED(iter);
  dlite_globals_set(state);
  return &dlite_json_plugin;
}
