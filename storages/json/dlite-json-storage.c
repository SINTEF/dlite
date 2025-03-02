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
  DLiteJsonFlag jflags; /* output flags */
  int fmt_given;        /* whether single/multi entity format is given */
  int changed;          /* whether the storage is changed */
  map_uuid_t ids;       /* maps uuids to ids */
} DLiteJsonStorage;


/** Returns default mode:
    - 'w': if we can't open `uri`
    - 'r': if `uri` is in single-entity format
    - 'a': otherwise
*/
static int default_mode(const char *uri, const unsigned char *buf, size_t size)
{
  int mode, stat;
  JStore *js = jstore_open();

  ErrTry:
    if (uri)
      stat = jstore_update_from_file(js, uri);
    else
      stat = jstore_update_from_string(js, (const char *)buf, size);
  ErrCatch(1):
    break;
  ErrEnd;

  mode = (stat) ? 'w' : (jstore_get(js, "properties")) ? 'r' : 'a';
  jstore_close(js);
  return mode;
}


/*
  Help function for loading json data. Either `uri` or `buf` should be given.
 */
DLiteStorage *json_loader(const DLiteStoragePlugin *api, const char *uri,
                          const unsigned char *buf, size_t size,
                          const char *options)
{
  DLiteJsonStorage *s=NULL;
  DLiteStorage *retval=NULL;
  char *mode_descr = "How to open storage.  Valid values are: "
    "\"r\" (read-only); "
    "\"w\" (truncate existing storage or create a new one); "
    "\"a\" (appends to existing storage or creates a new one)";
  DLiteOpt opts[] = {
    {'m', "mode",      "", mode_descr},
    {'s', "single",    "", "Whether to write single-entity format"},
    {'k', "uri-key",   "false", "Whether to use uri as json key"},
    {'u', "with-uuid", "false", "Whether to include uuid in output"},
    {'M', "with-meta", "false", "Always include meta in output"},
    {'a', "arrays",    "false", "Serialise metadata dimensions and properties as arrays"},
    {'d', "as-data",   "false", "Alias for `single=false` (deprecated)"},
    {'c', "compact",   "false", "Alias for `single` (deprecated)"},
    {'U', "useid",     "",      "Unused (deprecated)"},
    {0, NULL, NULL, NULL}
  };
  int load;  // whether to load uri

  /* parse options */
  char *optcopy = (options) ? strdup(options) : NULL;
  if (dlite_option_parse(optcopy, opts, 0)) goto fail;

  char mode = *opts[0].value;
  int single = (*opts[1].value) ? atob(opts[1].value) : -2;
  int urikey = atob(opts[2].value);
  int withuuid = atob(opts[3].value);
  int withmeta = atob(opts[4].value);
  int arrays = atob(opts[5].value);

  /* deprecated options */
  if (atob(opts[6].value) > 0) single = (warn("`asdata` is deprecated"), 0);
  if (atob(opts[7].value) > 0) single = (warn("`compact` is deprecated"), 1);
  if (atob(opts[8].value) > 0) warn("`useid` is deprecated");

  /* check options */
  if (single == -1) FAILCODE1(dliteOptionError,
                              "invalid boolean value for `single=%s`.",
                              opts[1].value);
  if (urikey < 0) FAILCODE1(dliteOptionError,
                            "invalid boolean value for `uri-key=%s`.",
                            opts[2].value);
  if (withuuid < 0) FAILCODE1(dliteOptionError,
                              "invalid boolean value for `with-uuid=%s`.",
                              opts[3].value);
  if (withmeta < 0) FAILCODE1(dliteOptionError,
                              "invalid boolean value for `with-meta=%s`.",
                              opts[4].value);
  if (arrays < 0) FAILCODE1(dliteOptionError,
                            "invalid boolean value for `arrays=%s`.",
                            opts[5].value);

  if (!(s = calloc(1, sizeof(DLiteJsonStorage))))
   FAILCODE(dliteMemoryError, "allocation failure");
  s->api = api;

  if (!mode)
    mode = default_mode(uri, buf, size);
  s->flags |= dliteGeneric;
  switch (mode) {
  case 'r':
    load = 1;
    s->flags |= dliteReadable;
    s->flags &= ~dliteWritable;
    break;
  case 'a':
    if (single > 0) FAILCODE(dliteStorageSaveError,
                             "cannot append in single-entity format");
    load = 1;
    s->flags |= dliteReadable;
    s->flags |= dliteWritable;
    break;
  case 'w':
    load = 0;
    s->flags &= ~dliteReadable;
    s->flags |= dliteWritable;
    break;
  default:
    FAILCODE1(dliteOptionError,
          "invalid \"mode\" value: '%c'. Must be \"r\" (read-only), "
          "\"w\" (write) or \"a\" (append)", mode);
  }

  s->fmt_given = (single >= 0) ? 1 : 0;
  if (single > 0) s->jflags |= dliteJsonSingle;
  if (urikey) s->jflags |= dliteJsonUriKey;
  if (withuuid) s->jflags |= dliteJsonWithUuid;
  if (withmeta) s->jflags |= dliteJsonWithMeta;
  if (arrays) s->jflags |= dliteJsonArrays;

  /* Load jstore if not in write mode */
  if (load) {
    DLiteJsonFormat fmt;
    if (!(s->jstore = jstore_open())) goto fail;
    if (uri)
      fmt = dlite_jstore_loadf(s->jstore, uri);
    else
      fmt = dlite_jstore_loads(s->jstore, (const char *)buf, size);
    if (fmt < 0) goto fail;
    if (fmt == dliteJsonMetaFormat && mode != 'a') s->flags &= ~dliteWritable;
  }

  retval = (DLiteStorage *)s;

 fail:
  if (optcopy) free(optcopy);
  if (!retval && s) dlite_storage_close((DLiteStorage *)s);

  return retval;
}



/**
  Returns an url to the metadata.

  Valid `options` are:

  - mode : r | w | a
      Valid values are:
      - r   Open existing file for read-only
      - w   Truncate existing file or create new file
      - a   Append to existing file or create new file (default)
  - single : yes | no
      Whether to write single-entity format.
  - uri-key : yes | no
      Whether to use URI (if it exists) as json key instead of UUID
  - with-uuid : yes | no
      Whether to include uuid in output.
  - with-meta : yes | no
      Whether to always include meta in output (even for metadata).
  - arrays : yes | no
      Whether to write metadata dimensions and properties as arrays.
  - as-data : yes | no (deprecated)
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
  return json_loader(api, uri, NULL, 0, options);
}


/**
  Closes data handle json. Returns non-zero on error.
 */
int json_close(DLiteStorage *s)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  int stat=0;
  if (js->jstore) {
    if (js->flags & dliteWritable && js->changed)
      stat = jstore_to_file(js->jstore, js->location);
    stat |= jstore_close(js->jstore);
  }
  return stat;
}


/**
  Load instance `id` from storage `s` and return it.
  NULL is returned on error.
 */
DLiteInstance *json_load(const DLiteStorage *s, const char *id)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  const char *buf=NULL, *scanid;
  DLiteIdType idtype;
  char uuid[DLITE_UUID_LENGTH+1];

  if (!js->jstore) {
    if (s->location)
      FAILCODE1(dliteStorageLoadError,
                "cannot load JSON file: \"%s\"", s->location);
    else
      FAILCODE(dliteStorageLoadError, "cannot load JSON buffer");
  }

  if (!id || !*id) {
    JStoreIter iter;
    if (jstore_iter_init(js->jstore, &iter)) goto fail;
    if (!(id = jstore_iter_next(&iter)))
      FAILCODE1(dliteStorageLoadError,
            "cannot load instance from empty storage \"%s\"", s->location);
    if (jstore_iter_next(&iter)) {
      FAILCODE1(dliteStorageLoadError,
            "id is required when loading from storage with more "
            "than one instance: %s", s->location);
    }
    if (jstore_iter_deinit(&iter)) goto fail;
  } else if ((idtype = dlite_get_uuid(uuid, id)) && idtype != dliteIdRandom) {
    buf = jstore_get(js->jstore, uuid);
  }
  if (!buf && !(buf = jstore_get(js->jstore, id)))
      goto fail;
  if (dlite_get_uuid(uuid, id) == dliteIdCopy) {
    /* the provided id is an uuid - check if a human readable id has been
       assoicated with `id` as a label */
    if (!(scanid = jstore_get_label(js->jstore, id))) scanid = id;
  } else {
    scanid = id;
  }
  return dlite_json_sscan(buf, scanid, NULL);
 fail:
  return NULL;
}


/**
  Saves instance `inst` to storage `s`.  Returns non-zero on error.
*/
int json_save(DLiteStorage *s, const DLiteInstance *inst)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  int stat=1;
  DLiteJsonFlag jflags = js->jflags;

  if (!(s->flags & dliteWritable))
    FAILCODE1(dliteStorageSaveError,
              "storage \"%s\" is not writable", s->location);

  /* If single/multi format is not given, infer it from `inst` */
  if (!js->fmt_given && dlite_instance_is_meta(inst))
    jflags |= dliteJsonSingle;

  if (jflags & dliteJsonSingle) {
    if (js->changed)
      FAILCODE1(dliteStorageSaveError,
                "Trying to save more than once in single-entity format: %s",
                s->location);
    int n = dlite_json_printfile(s->location, inst, jflags);
    stat = (n > 0) ? 0 : 1;
  } else {
    if (!js->jstore && !(js->jstore = jstore_open())) goto fail;
    stat = dlite_jstore_add(js->jstore, inst, jflags);
  }
  js->changed = 1;
 fail:
  return stat;
}


/**
  Load instance `id` from buffer `buf` of size `size`.
  Returns NULL on error.
 */
DLiteInstance *json_memload(const DLiteStoragePlugin *api,
                            const unsigned char *buf, size_t size,
                            const char *id, const char *options)
{
  DLiteInstance *inst;
  DLiteStorage *s = json_loader(api, NULL, buf, size, options);
  if (!s) return NULL;
  inst = json_load(s, id);
  json_close(s);
  free(s);
  return inst;
}


/**
  Save instance `inst` to buffer `buf` of size `size`.

  Returns number of bytes written to `buf` (or would have been written
  to `buf` if `buf` is not large enough).
  Returns a negative error code on error.
 */
int json_memsave(const DLiteStoragePlugin *api,
                 unsigned char *buf, size_t size,
                 const DLiteInstance *inst, const char *options)
{
  int retval=-1, indent;
  DLiteJsonFlag flags=0;
  DLiteOpt opts[] = {
    {'i', "indent",    "0",     "Indentation."},
    {'s', "single",    "",      "Whether to write in single-entity format."},
    {'k', "uri-key",   "false", "Whether to use uri as json key."},
    {'u', "with-uuid", "false", "Whether to include uuid in output."},
    {'M', "with-meta", "false", "Always include meta in output."},
    {'a', "arrays",    "false", "Serialise metadata dims and props as arrays."},
    {'n', "no-parent", "false", "Do not write transaction parent info."},
    {'c', "compact",   "false", "Write relations with no newline."},
    {0, NULL, NULL, NULL}
  };
  char *optcopy = (options) ? strdup(options) : NULL;
  UNUSED(api);
  if (dlite_option_parse(optcopy, opts, 0)) goto fail;
  indent = atoi(opts[0].value);
  if ((*opts[1].value) ? atob(opts[1].value) : dlite_instance_is_meta(inst))
    flags |= dliteJsonSingle;
  if (atob(opts[2].value)) flags |= dliteJsonUriKey;
  if (atob(opts[3].value)) flags |= dliteJsonWithUuid;
  if (atob(opts[4].value)) flags |= dliteJsonWithMeta;
  if (atob(opts[5].value)) flags |= dliteJsonArrays;
  if (atob(opts[6].value)) flags |= dliteJsonNoParent;
  if (atob(opts[7].value)) flags |= dliteJsonCompactRel;
  retval = dlite_json_sprint((char *)buf, size, inst, indent, flags);
 fail:
  if (optcopy) free(optcopy);
  return retval;
}


/**
  Creates and returns a new iterator used by dlite_json_iter_next().

  If `metaid` is not NULL, dlite_json_iter_next() will only iterate over
  instances whos metadata corresponds to this id.

  It is an error if this function is called in single-entity mode.

  Returns new iterator or NULL on error.
 */
void *json_iter_create(const DLiteStorage *s, const char *metaid)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  if (!js->jstore)
    return errx(dliteStorageLoadError,
                "iteration not possible in write mode"), NULL;
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
  NULL,                     /* flush */
  NULL,                     /* help */

  /* query api */
  json_iter_create,         /* iterCreate */
  json_iter_next,           /* iterNext */
  json_iter_free,           /* iterFree */

  /* direct api */
  json_load,                /* loadInstance */
  json_save,                /* saveInstance */
  NULL,                     /* deleteInstance */

  /* In-memory API */
  json_memload,             /* memLoadInstance */
  json_memsave,             /* memSaveInstance */

  /* === API to deprecate === */
  NULL,                     /* getUUIDs */

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
get_dlite_storage_plugin_api(void *globals, int *iter)
{
  UNUSED(iter);
  dlite_globals_set(globals);
  return &dlite_json_plugin;
}
