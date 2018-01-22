#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compat.h"
#include "dlite.h"
#include "dlite-datamodel.h"
#include "getuuid.h"
#include "err.h"


/* Convenient macros for failing */
#define FAIL(msg) do { err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { err(1, msg, a1); goto fail; } while (0)


/* NULL-terminated array of all backends */
extern DLitePlugin h5_plugin;
DLitePlugin *plugin_list[] = {
  &h5_plugin,
  NULL
};


/* Returns a pointer to API for driver or NULL on error. */
static DLitePlugin *get_plugin(const char *driver)
{
  DLitePlugin *plugin;
  for (plugin=plugin_list[0]; plugin; plugin++)
    if (strcmp(plugin->name, driver) == 0)
      break;
  if (!plugin) errx(1, "invalid driver: '%s'", driver);
  return plugin;
}


/********************************************************************
 * Public api
 ********************************************************************/

/*
  Opens data item `id` from `uri` using `driver`.
  Returns a opaque pointer to a data handle or NULL on error.

  The `options` are passed to the driver.
 */
DLiteStorage *dlite_storage_open(const char *driver, const char *uri,
                                 const char *options)
{
  DLitePlugin *api;
  DLiteStorage *storage=NULL;

  if (!(api = get_plugin(driver))) goto fail;
  if (!(storage = api->open(uri, options))) goto fail;

  storage->api = api;
  if (!(storage->uri = strdup(uri))) FAIL(NULL);

  return storage;
 fail:
  if (storage) free(storage);
  return NULL;
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
  free(storage);
  return stat;
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
