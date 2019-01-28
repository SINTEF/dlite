#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils/compat.h"
#include "utils/err.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-datamodel.h"
#include "dlite-storage-plugins.h"
#include "getuuid.h"



/********************************************************************
 * Public api
 ********************************************************************/

/*
  Opens a storage located at `uri` using `driver`.
  Returns a opaque pointer or NULL on error.

  The `options` are passed to the driver.
 */
DLiteStorage *dlite_storage_open(const char *driver, const char *uri,
                                 const char *options)
{
  const DLiteStoragePlugin *api;
  DLiteStorage *storage=NULL;

  if (!(api = dlite_storage_plugin_get(driver))) goto fail;
  if (!(storage = api->open(uri, options))) goto fail;

  storage->api = api;
  if (!(storage->uri = strdup(uri))) FAIL(NULL);
  if (options && !(storage->options = strdup(options))) FAIL(NULL);
  storage->idflag = dliteIDTranslateToUUID;

  return storage;
 fail:
  if (storage) free(storage);
  return NULL;
}


/*
  Like dlite_storage_open(), but takes as input an url of the form
  ``driver://uri?options``.  The question mark and options may be left out.

  Returns a new storage, or NULL on error.
*/
DLiteStorage *dlite_storage_open_url(const char *url)
{
  char *url2=strdup(url), *driver=NULL, *location=NULL, *options=NULL;
  DLiteStorage *s;
  if (dlite_split_url(url2, &driver, &location, &options, NULL)) return NULL;
  s = dlite_storage_open(driver, location, options);
  free(url2);
  return s;
}


/*
   Closes data handle `d`. Returns non-zero on error.
*/
int dlite_storage_close(DLiteStorage *storage)
{
  int stat;
  assert(storage);
  stat = storage->api->close(storage);
  free(storage->uri);
  if (storage->options) free(storage->options);
  free(storage);
  return stat;
}


/*
  Returns the current mode of how to handle instance IDs.
 */
DLiteIDFlag dlite_storage_get_idflag(const DLiteStorage *s)
{
  return s->idflag;
}

/*
  Sets how instance IDs are handled.
 */
void dlite_storage_set_idflag(DLiteStorage *s, DLiteIDFlag idflag)
{
  s->idflag = idflag;
}


/*
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array.
 */
char **dlite_storage_uuids(const DLiteStorage *s)
{
  if (!s->api->getUUIDs)
    return errx(1, "driver '%s' does not support getUUIDs()",
                s->api->name), NULL;
  return s->api->getUUIDs(s);
}


/*
  Frees NULL-terminated array of instance names returned by
  dlite_storage_uuids().
*/
void dlite_storage_uuids_free(char **names)
{
  char **p;
  if (!names) return;
  for (p=names; *p; p++) free(*p);
  free(names);
}


/*
  Returns non-zero if storage `s` is writable.
 */
int dlite_storage_is_writable(const DLiteStorage *s)
{
  return s->writable;
}
