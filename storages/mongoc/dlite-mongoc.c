#include <mongoc/mongoc.h>
#include <bson/bson.h>

#include "dlite.h"
#include "dlite-storage-plugins.h"
#include "dlite-macros.h"


/* Storage for BSON backend */
typedef struct {
  DLiteStorage_HEAD
  mongoc_client_t *client;
  mongoc_collection_t *collection;
  bson_t *document;
  //bson_oid_t oid;
  char *database;
  char *coll;
} DLiteMongocStorage;


/* */
DLiteStorage *dlite_mongoc_open(const DLiteStoragePlugin *api, const char *uri,
                                const char *options)
{
  DLiteMongocStorage *sm=NULL;
  char *mode_descr = "How to open storage.  Valid values are: "
    "\"r\" (read-only); "
    "\"w\" (truncate existing storage or create a new one); "
    "\"a\" (appends to existing storage or creates a new one)";
  DLiteOpt opts[] = {
    {'m', "mode",      "a",  "r: read-only, w: overwrite, a: append"},
    {'d', "database",  "",   "Database name."},
    {'u', "coll",      "",   "Collection name."},
    {0, NULL, NULL, NULL}
  };

  /* parse options */
  char *optcopy = (options) ? strdup(options) : NULL;
  if (dlite_option_parse(optcopy, opts, 1)) goto fail;

  char mode            = *opts[0].value;
  const char *database = opts[1].value;
  const char *coll     = opts[2].value;

  if (!(sm = calloc(1, sizeof(DLiteMongocStorage)))) FAIL("allocation failure");
  sm->api = api;

  sm->flags |= dliteGeneric;
  switch (mode) {
  case 'r':
    sm->flags |= dliteReadable;
    sm->flags &= ~dliteWritable;
    break;
  case 'a':
    sm->flags |= dliteReadable;
    sm->flags |= dliteWritable;
    break;
  case 'w':
    sm->flags &= ~dliteReadable;
    sm->flags |= dliteWritable;
    break;
  default:
    FAIL1("invalid \"mode\" value: '%c'. Must be \"r\" (read-only), "
          "\"w\" (write) or \"a\" (append)", mode);
  }

  if (!(s->client = mongoc_client_new(uri)))
    FAIL1("cannot create MongoDB client for \"%s\"", uri);
  if (!(s->collection = mongoc_client_get_collection(s->client, s->database,
                                                     s->coll)))
    FAIL2("cannot create MongoDB collection for db \"%s\": %s",
          s->database, s->coll);
  if (!(s->document = bson_new()))
    FAIL("cannot create new BSON document");
  if (!(s->database = strdup(database))) FAIL("allocation error");
  if (!(s->coll = strdup(coll))) FAIL("allocation error");

  return s;
 fail:
  if (s) mongoc_close(s);
  return NULL;
}


/* Close mongoc storage.  Returns non-zero on error. */
int dlite_mongoc_close(DLiteStorage *s)
{
  if (s->client) mongoc_client_destroy(s->client);
  if (s->collection) mongoc_collection_destroy(s->collection);
  if (s->database) free(s->database);
  if (s->coll) free(s->coll);
  free(s);
}


DLiteInstance *dlite_mongoc_load(const DLiteStorage *s, const char *id)
{
  DLiteMongocStorage *ms = (DLiteMontocStorage *)s;
  char uuid[DLITE_UUID_LENGTH+1];
  DLiteInstance *inst=NULL;

  if (dlite_get_uuid(uuid, id) < 0) goto fail;

  return inst;
 fail:
  return NULL;
}
