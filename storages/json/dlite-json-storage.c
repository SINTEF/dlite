/* dlite-json-storage.c -- DLite plugin for json */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "config.h"

#include "utils/err.h"
#include "utils/strtob.h"
#include "dlite.h"
#include "dlite-storage-plugins.h"
#include "dlite-macros.h"


/** Storage for json backend. */
typedef struct {
  DLiteStorage_HEAD
  DLiteJsonFlag flags;  /* output flags */
  int append;           /* whether to append to existing output */
} DLiteJsonStorage;


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
DLiteStorage *dlite_json_open(const DLiteStoragePlugin *api, const char *uri,
                              const char *options)
{
  DLiteJsonStorage *s;
  DLiteStorage *retval=NULL;
  char *mode_descr = "How to open storage.  Valid values are: "
    "\"r\" (read-only); "
    "\"w\" (truncate existing storage or create a new one); "
  "\"a\" (appends to existing storage or creates a new one, default)";
  DLiteOpt opts[] = {
    {'m', "mode",      "a", mode_descr},
    {'u', "with-uuid", "false", "Whether to include uuid in output"},
    {'d', "as-data",   "false", "Whether to write metadata as data"},
    {'c', "compact",   "false", "Aliad for `as-data=false` (deprecated)"},
    {'M', "meta",      "false", "Alias for `with-uuid` (deprecated)"},
    {'U', "useid",     "",      "Unused (deprecated)"},
    {0, NULL, NULL, NULL}
  };
  char *optcopy = (options) ? strdup(options) : NULL;
  const char **mode = &opts[0].value;
  int withuuid = atob(opts[1].value);
  int asdata = atob(opts[2].value);

  if (!(s = calloc(1, sizeof(DLiteJsonStorage)))) FAIL("allocation failure");
  s->api = api;
  s->location = strdup(uri);
  if (options) s->options = strdup(options);

  /* parse options */
  if (dlite_option_parse(optcopy, opts, 1)) goto fail;
  if (strcmp(*mode, "r") == 0) {
    s->writable = 0;
  } else if (strcmp(*mode, "w") == 0) {
    s->writable = 1;
  } else if (strcmp(*mode, "a") == 0 || strcmp(*mode, "append") == 0) {
    s->append = 1;
    s->writable = 1;
  } else {
    FAIL1("invalid \"mode\" value: '%s'. Must be \"r\" (read-only), "
          "\"w\" (write) or \"a\" (append)", *mode);
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
  if (!retval && s) free(s);

  return retval;
}


/**
  Closes data handle json. Returns non-zero on error.
 */
int dlite_json_close(DLiteStorage *s)
{
  UNUSED(s);
  return 0;
}


/**
  Creates and returns a new iterator used by dlite_json_iter_next().

  If `metaid` is not NULL, dlite_json_iter_next() will only iterate over
  instances whos metadata corresponds to this id.

  Returns new iterator or NULL on error.
 */
void *dlite_json_iter_create(const DLiteStorage *s, const char *metaid)
{
  FILE *fp;
  char *buf;
  DLiteJsonIter *iter;
  if (!(fp = fopen(s->location, "r")))
    FAIL1("cannot open storage \"%s\"", s->location);
  if (!(buf = fu_readfile(fp))) goto fail;
  if (!(iter = dlite_json_iter_init(buf, 0, metaid))) goto fail;
 fail:
  if (fp) fclose(fp);
  if (buf) free(buf);
  printf("*** create iter=%p\n", (void *)iter);
  return iter;
}


/**
  Writes the uuid of the next instance to `buf`, where `iter` is an
  iterator returned by dlite_json_iter_create().

  Return non-zero on error.
 */
int dlite_json_iter_next(void *iter, char *buf)
{
  int len;
  const char *id;
  if (!(id = dlite_json_next(iter, &len))) return 1;
  if (dlite_get_uuid(buf, id) < 0) return 1;
  return 0;
}


/**
  Free's iterator created with dlite_json_iter_create().
 */
void dlite_json_iter_free(void *iter)
{
  printf("*** free iter=%p\n", (void *)iter);
  dlite_json_iter_deinit(iter);
}


/**
  Load instance `id` from storage `s` and return it.
  NULL is returned on error.
 */
DLiteInstance *dlite_json_load(const DLiteStorage *s, const char *id)
{
  return dlite_json_scanfile(s->location, id, NULL);
}


/**
  Saves instance `inst` to storage `s`.  Returns non-zero on error.
*/
int dlite_json_save(DLiteStorage *s, const DLiteInstance *inst)
{
  DLiteJsonStorage *js = (DLiteJsonStorage *)s;
  int retval=1;
  char *buf=NULL;
  size_t size=0;
  FILE *fp=NULL;
  if (!s->writable)
    FAIL1("storage \"%s\" is not writable", s->location);
  if (!(fp = fopen(s->location, "w")))
    FAIL1("cannot open storage \"%s\" for writing", s->location);
  if (js->append) {
    if (dlite_json_append(&buf, &size, inst, 0) < 0) goto fail;
  } else {
    if (dlite_json_asprint(&buf, &size, 0, inst, 0, js->flags) < 0)
      goto fail;
  }
  if (fprintf(fp, buf) < 0)
    FAIL1("error writing to storage \"%s\"", s->location);
  retval = 0;
 fail:
  if (buf) free(buf);
  if (fp) fclose(fp);
  return retval;
}


static DLiteStoragePlugin dlite_json_plugin = {
  /* head */
  "json",                   /* name */
  NULL,                     /* freeapi */

  /* basic api */
  dlite_json_open,          /* open */
  dlite_json_close,         /* close */

  /* queue api */
  dlite_json_iter_create,   /* iterCreate */
  dlite_json_iter_next,     /* iterNext */
  dlite_json_iter_free,     /* iterFree */
  NULL,                     /* getUUIDs */

  /* direct api */
  dlite_json_load,          /* loadInstance */
  dlite_json_save,          /* saveInstance */

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
get_dlite_storage_plugin_api(int *iter)
{
  UNUSED(iter);
  return &dlite_json_plugin;
}
