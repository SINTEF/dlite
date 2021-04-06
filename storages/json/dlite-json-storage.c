/* dlite-json-storage.c -- DLite plugin for json */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <jansson.h>
#include "json-utils.h"
#include "str.h"

#include "config.h"

#include "boolean.h"
#include "utils/strtob.h"
#include "utils/err.h"

#include "dlite-misc.h"
#include "dlite-schemas.h"

/*
*/
#include "dlite.h"
#include "dlite-datamodel.h"


/* Macro for getting rid of unused parameter warnings... */
#define UNUSED(x) (void)(x)

/* Convinient error macros */
#define FAIL0(msg) \
  do {err(-1, msg); goto fail;} while (0)

#define FAIL1(msg, a1) \
  do {err(-1, msg, a1); goto fail;} while (0)

#define FAIL2(msg, a1, a2) \
  do {err(-1, msg, a1, a2); goto fail;} while (0)

#define FAIL3(msg, a1, a2, a3) \
  do {err(-1, msg, a1, a2, a3); goto fail;} while (0)

#define FAIL4(msg, a1, a2, a3, a4) \
  do {err(1, msg, a1, a2, a3, a4); goto fail; } while (0)

#define FAIL5(msg, a1, a2, a3, a4, a5) \
  do {err(1, msg, a1, a2, a3, a4, a5); goto fail; } while (0)


/* Error macros for when DLiteDataModel instance d in available */
#define DFAIL0(d, msg) \
  do {err(-1, "%s/%s: " msg, d->s->location, d->uuid); \
    goto fail;} while (0)

#define DFAIL1(d, msg, a1) \
  do {err(-1, "%s/%s: " msg, d->s->location, d->uuid, a1); \
    goto fail;} while (0)

#define DFAIL2(d, msg, a1, a2) \
  do {err(-1, "%s/%s: " msg, d->s->location, d->uuid, a1, a2); \
    goto fail;} while (0)

#define DFAIL3(d, msg, a1, a2, a3) \
  do {err(-1, "%s/%s: " msg, d->s->location, d->uuid, a1, a2, a3); \
    goto fail;} while (0)

#define DFAIL4(d, msg, a1, a2, a3, a4)                            \
  do {err(-1, "%s/%s: " msg, d->s->location, d->uuid, a1, a2, a3, a4); \
    goto fail;} while (0)



/* How the json-data is organised */
typedef enum {
  fmtNormal,      /* format output as normal data */
  fmtMeta,        /* format output as SOFT metadata */
  fmtSchema       /* schemas or meta-metadata */
} DataFormat;

/* Storage for json backend. */
typedef struct {
  DLiteStorage_HEAD
  json_t *root;       /* json root object */
  int compact;        /* whether to write output in compact format */
  DataFormat fmt;     /* layout of json-data */
} DLiteJsonStorage;

/* Data model for json backend. */
typedef struct {
  DLiteDataModel_HEAD
  json_t *instance;     /* json object to instance, borrowed reference */
  json_t *meta;         /* json object to metadata, borrowed reference */
  json_t *dimensions;   /* json object to dimensions, borrowed reference */
  json_t *properties;   /* json object to properties, borrowed reference */
  json_t *relations;    /* json object to relations, borrowed reference */
  DataFormat fmt;       /* layout of json-data */
} DLiteJsonDataModel;


/*
  Returns a string value or NULL on error.
 */
const char *object_get_string(json_t *obj, const char *key)
{
  const char *retval=NULL;
  json_t *value = json_object_get(obj, key);
  if (json_is_string(value))
    retval = json_string_value(value);
  return retval;
}

void object_set_string(json_t *obj, const char *key, const char *val)
{
  json_t *value = json_object_get(obj, key);
  if (json_is_string(value))
    json_string_set(value, val);
  else {
    value = json_string(val);
    json_object_set_new(obj, key, value);
  }
}

int object_get_integer(json_t *obj, const char *key)
{
  int retval=0;
  json_t *value = json_object_get(obj, key);
  if (json_is_integer(value))
     retval = json_integer_value(value);
  return retval;
}

void object_set_integer(json_t *obj, const char *key, const int val)
{
  json_t *value = json_object_get(obj, key);
  if (json_is_integer(value))
    json_integer_set(value, val);
  else {
    value = json_integer(val);
    json_object_set_new(obj, key, value);
  }
}

void object_set_real(json_t *obj, const char *key, const double val)
{
  json_t *value = json_object_get(obj, key);
  if (json_is_real(value))
    json_real_set(value, val);
  else {
    value = json_real(val);
    json_object_set_new(obj, key, value);
  }
}

/**
  Returns an url to the metadata.

  Valid `options` are:

  - mode : append | r | w
      Valid values are:
      - append   Append to existing file or create new file (default)
      - r        Open existing file for read-only
      - w        Truncate existing file or create new file
  - compact : yes | no
      Whether to write output in compact format
  - meta : yes | no
      Whether to format output as metadata
  - useid: translate | require | keep
      How to use the ID
 */
DLiteStorage *
dlite_json_open(const DLiteStoragePlugin *api, const char *uri,
                const char *options)
{
  DLiteJsonStorage *s;
  DLiteStorage *retval=NULL;
  json_error_t error;
  size_t n;
  char *mode_descr = "How to open storage.  Valid values are: "
    "\"append\" (appends to existing storage or creates a new one); "
    "\"r\" (read-only); "
    "\"w\" (truncate existing storage or create a new one)";
  char *useid_descr = "How to use ID.  Valid values are: "
    "\"translate\" (translate to UUID if ID is not a valid UUID); "
    "\"require\" (require a valid UUID as ID); "
    "\"keep\" (keep ID as it is)";
  DLiteOpt opts[] = {
    {'m', "mode",    "append", mode_descr},
    {'c', "compact", "false",  "Whether to write output in compact format"},
    {'M', "meta",    "false",  "Whether to format output as metadata"},
    {'u', "useid",   "translate",  useid_descr},
    {0, NULL, NULL, NULL}
  };
  char *optcopy = (options) ? strdup(options) : NULL;
  const char **mode = &opts[0].value;
  int meta;
  UNUSED(api);

  if (!(s = calloc(1, sizeof(DLiteJsonStorage)))) FAIL0("allocation failure");

  /* parse options */
  if (dlite_option_parse(optcopy, opts, 1)) goto fail;
  if (strcmp(*mode, "append") == 0) {  /* default */
    FILE *fp;
    if ((fp = fopen(uri, "r"))) {  /* `uri` exists */
      fclose(fp);
      s->root = json_load_file(uri, 0, &error);
    } else {  /* `uri` doesn't exists */
      s->root = json_object();
    }
    s->writable = 1;
  } else if (strcmp(*mode, "r") == 0) {
    s->root = json_load_file(uri, 0, &error);
    s->writable = 0;
  } else if (strcmp(*mode, "w") == 0) {
    s->root = json_object();
    s->writable = 1;
  } else {
    FAIL1("invalid \"mode\" value: '%s'. Must be \"append\", \"r\" "
          "(read-only) or \"w\" (write)", *mode);
  }

  if ((s->compact = atob(opts[1].value)) == -1)
    errx(1, "invalid boolean value: '%s'.  Assuming true.", opts[1].value);

  if ((meta = atob(opts[2].value)) == 0)
    s->fmt = fmtNormal;
  else {
    s->fmt = fmtMeta;
    if (meta < 0)
      errx(1, "invalid boolean value: '%s'.  Assuming true.", opts[2].value);
  }

  if (strcmp(opts[3].value, "require") == 0) {
    s->idflag = dliteIDRequireUUID;
  } else if (strcmp(opts[3].value, "keep") == 0) {
    s->idflag = dliteIDKeepID;
  } else {
    s->idflag = dliteIDTranslateToUUID;
  }

  if (s->root == NULL) {
    n = strlen(error.text);
    if (n > 0)
      err(1, "JSON parse error on line %d: %s\n", error.line, error.text);
    else
      err(1, "JSON parse error on line %d\n", error.line);
    FAIL2("cannot open: '%s' with options '%s'", uri, options);
  }
  if (!json_is_object(s->root))
    FAIL1("expected an object as root in json file: '%s'", uri);

  retval = (DLiteStorage *)s;

 fail:
  if (optcopy) free(optcopy);
  if (!retval && s) {
    if (s->root) json_decref(s->root);
    free(s);
  }
  return retval;
}

/**
  Closes data handle json. Returns non-zero on error.
 */
int dlite_json_close(DLiteStorage *s)
{
  DLiteJsonStorage *storage = (DLiteJsonStorage *)s;
  int nerr=0;
  if (storage->writable == 1) {
    size_t flags=0;
    flags |= (storage->compact) ? JSON_COMPACT : JSON_INDENT(2);
    json_dump_file(storage->root, storage->location, flags);
  }
  json_decref(storage->root);
  return nerr;
}

/**
  Creates a new datamodel for json storage `s`.

  If `id` exists in the root of `s`, the datamodel should describe the
  corresponding instance.

  Returns the new datamodel or NULL on error.
*/
DLiteDataModel *dlite_json_datamodel(const DLiteStorage *s, const char *id)
{
  DLiteJsonDataModel *d=NULL;
  DLiteDataModel *retval=NULL;
  DLiteJsonStorage *storage = (DLiteJsonStorage *)s;
  char uuid[DLITE_UUID_LENGTH + 1];
  json_t *data=NULL;
  json_t *jname, *jver, *jns;

  if (!(d = calloc(1, sizeof(DLiteJsonDataModel))))
    FAIL0("allocation failure");

  if (id) {
    if (dlite_storage_get_idflag(s) == dliteIDKeepID) {
      data = json_object_get(storage->root, id);
    } else  {
      if (dlite_get_uuid(uuid, id) < 0) goto fail;
      data = json_object_get(storage->root, uuid);
    };
  }

  if (data) {
    /* Instance `id` exists - assign datamodel... */
    if (!json_is_object(data))
      FAIL2("expected a json object for instance '%s' in '%s'",
            id, storage->location);
    d->instance = data;
    d->meta = json_object_get(data, "meta");
    d->dimensions = json_object_get(data, "dimensions");
    d->properties = json_object_get(data, "properties");
    d->relations = json_object_get(data, "relations");
    if (!d->properties) {
      d->properties = data;
    }

  } else if ((jns = json_object_get(storage->root, "namespace")) &&
             (jver = json_object_get(storage->root, "version")) &&
             (jname = json_object_get(storage->root, "name"))) {

    if (id && !storage->writable) {
      char uuid2[DLITE_UUID_LENGTH + 1];
      const char *name = json_string_value(jname);
      const char *version = json_string_value(jver);
      const char *namespace = json_string_value(jns);
      char *uri2 = dlite_join_meta_uri(name, version, namespace);
      if (dlite_get_uuid(uuid2, uri2) < 0) goto fail;
      free(uri2);
      if (strcmp(uuid2, uuid) != 0)
	FAIL5("uri %s/%s/%s in storage %s doesn't match id: %s",
	      namespace, version, name, storage->location, id);
    }

    /* Instance is a metadata definition */
    data = storage->root;
    d->instance = data;
    //if (!(d->meta = json_object_get(data, "meta"))) d->fmt = fmtMeta;
    d->fmt = fmtMeta;
    d->meta = json_object_get(data, "meta");
    d->dimensions = json_object_get(data, "dimensions");
    d->properties = json_object_get(data, "properties");
    d->relations = json_object_get(data, "relations");

  } else if ((jns = json_object_get(storage->root, "schema_namespace")) &&
	     (jver = json_object_get(storage->root, "schema_version")) &&
             (jname = json_object_get(storage->root, "schema_name"))) {

    if (id && !storage->writable) {
      char uuid2[DLITE_UUID_LENGTH + 1];
      const char *name = json_string_value(jname);
      const char *version = json_string_value(jver);
      const char *namespace = json_string_value(jns);
      char *uri2 = dlite_join_meta_uri(name, version, namespace);
      if (dlite_get_uuid(uuid2, uri2) < 0) goto fail;
      free(uri2);
      if (strcmp(uuid2, uuid) != 0)
	FAIL5("uri %s/%s/%s in storage %s doesn't match id: %s",
	      namespace, version, name, storage->location, id);
    }

    /* Instance is a meta-metadata definition (schema) */
    data = storage->root;
    d->instance = data;
    //if (!(d->meta = json_object_get(data, "meta"))) d->fmt = fmtSchema;
    d->fmt = fmtSchema;
    d->meta = json_object_get(data, "meta");
    d->dimensions = json_object_get(data, "schema_dimensions");
    d->properties = json_object_get(data, "schema_properties");
    d->relations = json_object_get(data, "schema_relations");

  } else {
    /* Instance `uuid` does not exists - create new instance and
       assign the datamodel... */
    if (!storage->writable)
      FAIL2("cannot create new instance '%s' in read-only storage %s",
            uuid, storage->location);
    d->fmt = storage->fmt;
    switch (d->fmt) {
    case fmtNormal:
      d->instance = json_object();
      json_object_set_new(storage->root, uuid, d->instance);
      d->meta = json_object();
      json_object_set_new(d->instance, "meta", d->meta);
      d->dimensions = json_object();
      json_object_set_new(d->instance, "dimensions", d->dimensions);
      d->properties = json_object();
      json_object_set_new(d->instance, "properties", d->properties);
      //d->relations = json_object();
      //json_object_set_new(d->instance, "relations", d->properties);
      break;
    case fmtMeta:
    case fmtSchema:
      d->instance = d->properties = storage->root;
      break;
    }
  }

  retval = (DLiteDataModel *)d;
 fail:
  if (!retval && d) free(d);
  return retval;
}

/**
  Closes data handle. Returns non-zero on error.
 */
int dlite_json_datamodel_free(DLiteDataModel *d)
{
  UNUSED(d);
  /* All json data is owned by the root node, which is released in
     dlite_json_close(). Nothing to free here... */
  return 0;
}


/**
  Returns pointer to (malloc'ed) metadata uri or NULL on error.
 */
char *dlite_json_get_metadata(const DLiteDataModel *d)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  const char *name=NULL;
  const char *version=NULL;
  const char *space=NULL;

  if (!data->meta) {
    switch (data->fmt) {
    case fmtMeta:     return strdup(DLITE_ENTITY_SCHEMA);
    case fmtSchema:   return strdup(DLITE_BASIC_METADATA_SCHEMA);
    default:
      return err(1, "unexpected json format: %d", data->fmt), NULL;
    }
  } else if (json_is_string(data->meta)) {
    return strdup(json_string_value(data->meta));
  } else if (json_is_object(data->meta)) {
    name = object_get_string(data->meta, "name");
    version = object_get_string(data->meta, "version");
    space = object_get_string(data->meta, "namespace");
    return dlite_join_meta_uri(name, version, space);
  } else {
    return errx(1, "invalid \"meta\" value"), NULL;
  }
}

void dlite_json_resolve_dimensions(DLiteDataModel *d, const DLiteMeta *meta)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  const DLiteProperty *prop;
  const DLiteDimension *dim;
  json_t *obj;
  size_t i;
  int j;
  ivec_t *shape = NULL;
  if (data->fmt == fmtNormal) {
    if (meta->_ndimensions != json_object_size(data->dimensions)) {
      if (!json_is_object(data->dimensions))
        data->dimensions = json_object();
      for(i=0; i < meta->_ndimensions; i++) {
        dim = dlite_meta_get_dimension_by_index(meta, i);
        obj = json_object_get(data->dimensions, dim->name);
        if (!obj) {
          json_object_set_new(data->dimensions, dim->name, json_integer(0));
        }
      }
      for(i=0; i < meta->_nproperties; i++) {
        prop = dlite_meta_get_property_by_index(meta, i);
        if (prop->ndims > 0) {
          obj = json_object_get(data->properties, prop->name);
          shape = json_array_dimensions(obj);
          if ((int)(ivec_size(shape)) == prop->ndims) {
            for(j=0; j < prop->ndims; j++) {
              obj = json_object_get(data->dimensions, prop->dims[j]);
              json_integer_set(obj, shape->data[j]);
            }
          }
        }
      }
    }
  }
  if (shape) ivec_free(shape);
}

/**
  Returns the size of dimension `name` or -1 on error.
 */
int dlite_json_get_dimension_size(const DLiteDataModel *d, const char *name)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  json_t *value;
  switch (data->fmt) {
  case fmtNormal:
    if (!(value = json_object_get(data->dimensions, name)))
      return err(-1, "no dimension named '%s'", name);
    if (!json_is_integer(value))
      return err(-1, "value of dimension '%s' is not an integer", name);
    return json_integer_value(value);

  case fmtMeta:
  case fmtSchema:
    if (strcmp(name, "ndimensions") == 0)
      return json_array_size(data->dimensions);
    else if (strcmp(name, "nproperties") == 0)
      return json_array_size(data->properties);
    else if (strcmp(name, "nrelations") == 0)
      return json_array_size(data->relations);
    else
      return err(-1, "expedted metadata dimension names are 'ndimensions', "
                 "'nproperties' or 'nrelations'; got '%s'", name);
  }
  assert(0);  /* never reached */
  return -1;
}


/* Recursive help function for handling n-dimensioal arrays */
static int getdim(size_t d, const json_t *arr, void **pptr,
                  DLiteType type, size_t size,
                  size_t ndims, const size_t *dims,
                  json_t *root)
{
  size_t i;
  if (d < ndims) {
    if (json_array_size(arr) != dims[d])
      return errx(1, "length of dimension %lu is %lu, expected %lu",
                  d, json_array_size(arr), dims[d]);
    for (i=0; i<dims[d]; i++) {
      const json_t *a = json_array_get(arr, i);
      if (getdim(d+1, a, pptr, type, size, ndims, dims, root)) return 1;
    }
  } else {
    if (dlite_json_get_value(*pptr, arr, type, size, root)) return 1;
    *((char **)pptr) += size;
  }
  return 0;
}


/**
  Copies property `name` to memory pointed to by `ptr`.
  Returns non-zero on error.
 */
int dlite_json_get_property(const DLiteDataModel *d, const char *name,
                            void *ptr, DLiteType type, size_t size,
                            size_t ndims, const size_t *dims)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  json_t *value;

  switch (data->fmt) {
  case fmtNormal:
    if (!(value = json_object_get(data->properties, name)))
      return errx(1, "no such key in json data: %s", name);
    break;
  case fmtMeta:
  case fmtSchema:
    if (!(value = json_object_get(data->instance, name)))
      return errx(1, "no such key in json data: %s", name);
    break;
  }

  if (ndims) {
    if (getdim(0, value, &ptr, type, size, ndims, dims, data->instance))
      return 1;
  } else {
    if (dlite_json_get_value(ptr, value, type, size, data->instance))
      return 1;
  }

  return 0;
}


/**
  Sets metadata.  Returns non-zero on error.
*/
int dlite_json_set_metadata(DLiteDataModel *d, const char *metadata)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  char *name, *version, *namespace;

  switch (data->fmt) {
  case fmtNormal:
    if (dlite_split_meta_uri(metadata, &name, &version, &namespace))
      return 1;
    object_set_string(data->meta, "name", name);
    object_set_string(data->meta, "version", version);
    object_set_string(data->meta, "namespace", namespace);
    free(name);
    free(version);
    free(namespace);
    break;
  case fmtMeta:
  case fmtSchema:
    object_set_string(data->instance, "meta", metadata);
    break;
  }
  return 0;
}

/**
  Sets size of dimension `name`.  Returns non-zero on error.
*/
int dlite_json_set_dimension_size(DLiteDataModel *d, const char *name, size_t size)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  object_set_integer(data->dimensions, name, (int)size);
  return 0;
}


/* A recursive help function to handle n-dimensional arrays. Args:
     d : current dimension
     arr : json array to fil out
     pptr : pointer to pointer to current data point
     size : size of each data point
     ndims : number of dimensions
     dims : length of each dimension (length: ndims)
*/
static json_t *setdim(size_t d, void **pptr,
                      DLiteType type, size_t size,
                      size_t ndims, const size_t *dims,
                      json_t *root)
{
  int i;
  json_t *item;
  if (d < ndims) {
    json_t *arr = json_array();
    for (i=0; i<(int)dims[d]; i++) {
      if (!(item = setdim(d + 1, pptr, type, size, ndims, dims, root)))
        return NULL;
      json_array_append_new(arr, item);
    }
    return arr;
  }  else {
    item = dlite_json_set_value(*pptr, type, size, root);
    *((char **)pptr) += size;
    return item;
  }
  assert(0);
}


/**
  Sets property `name` to the memory (of `size` bytes) pointed to by `ptr`.
  Returns non-zero on error.
*/
int dlite_json_set_property(DLiteDataModel *d, const char *name,
                            const void *ptr,
                            DLiteType type, size_t size,
                            size_t ndims, const size_t *dims)
{
  DLiteJsonDataModel *datamodel = (DLiteJsonDataModel *)d;
  json_t *item;
  if (ndims) {
    if (!(item = setdim(0, (void **)&ptr, type, size, ndims, dims,
                        datamodel->instance))) {
      return 1;
    }
    json_object_set_new(datamodel->properties, name, item);
  } else {
    if (!(item = dlite_json_set_value(ptr, type, size, datamodel->instance))) {
      return 1;
    }
    json_object_set_new(datamodel->properties, name, item);
  }
  return 0;
}


/**
  Returns a NULL-terminated array of string pointers to instance UUID's.
  The caller is responsible to free the returned array.
  Options is ignored.
*/
char **dlite_json_get_uuids(const DLiteStorage *s)
{
  DLiteJsonStorage *storage = (DLiteJsonStorage *)s;
  int i, n;
  char **names=NULL;
  const char *key;
  void *iter;
  size_t len;
  int ok = 1;
  const char *namespace, *version, *name;
  json_t *json;

  if ((json = json_object_get(storage->root, "name")) &&
      (name = json_string_value(json)) &&
      (json = json_object_get(storage->root, "version")) &&
      (version = json_string_value(json)) &&
      (json = json_object_get(storage->root, "namespace")) &&
      (namespace = json_string_value(json)) &&
      (names = calloc(2, sizeof(char *))) &&
      (names[0] = dlite_join_meta_uri(name, version, namespace)))
    return names;

  n = json_object_size(storage->root);
  if (n > 0) {
    if (!(names = malloc((n + 1)*sizeof(char *)))) {
      ok = 0;
    } else {
      iter = json_object_iter(storage->root);
      i = 0;
      while(iter) {
        key = json_object_iter_key(iter);
        len = strlen(key) + 1;
        if (!(names[i] = malloc(len))) {
          ok = 0;
          break;
        }
        memcpy(names[i], key, len);
        i++;
        iter = json_object_iter_next(storage->root, iter);
      }
      names[n] = NULL;
    }
  }

  if (ok == 0) {
    if (names) {
      for(i=0; i < n; i++)
        if (names[i])
          free(names[i]);
      free(names);
      names = NULL;
    }
  }

  return names;
}


/**
  Returns a positive value if dimension `name` is defined, zero if it
  isn't and a negative value on error.
 */
int dlite_json_has_dimension(const DLiteDataModel *d, const char *name)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  json_t *dim = json_object_get(data->dimensions, name);
  return dim != NULL;
}

/**
  Returns a positive value if property `name` is defined, zero if it
  isn't and a negative value on error.
 */
int dlite_json_has_property(const DLiteDataModel *d, const char *name)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  json_t *prop = json_object_get(data->properties, name);
  return prop != NULL;
}


/**
   If the uuid was generated from a unique name, return a pointer to a
   newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
char *dlite_json_get_dataname(const DLiteDataModel *d)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  return (char *)object_get_string(data->instance, "dataname");
}

/**
  Gives the instance a name.  This function should only be called
  if the uuid was generated from `name`.
  Returns non-zero on error.
*/
int dlite_json_set_dataname(DLiteDataModel *d, const char *name)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  object_set_string(data->instance, "dataname", name);
  return 0;
}


char *dlite_json_uri(json_t *obj)
{
  const char *name=NULL;
  const char *version=NULL;
  const char *namespace=NULL;
  const char *uri;
  int n = 0;

  if (json_is_object(obj)) {
    if ((uri = object_get_string(obj, "uri")) && !str_is_whitespace(uri))
      return (char *)uri;

    name = object_get_string(obj, "name");
    if (!str_is_whitespace(name))
      n++;
    version = object_get_string(obj, "version");
    if (!str_is_whitespace(version))
      n++;
    namespace = object_get_string(obj, "namespace");
    if (!str_is_whitespace(namespace))
      n++;

    if (n == 3)
      return dlite_join_meta_uri(name, version, namespace);
  }
  return NULL;
}

/* Assigns DLiteDimension from a json object */
int dlite_json_entity_dim(const json_t *obj, DLiteDimension *dim)
{
  if (!json_is_object(obj)) return 1;
  dim->name = str_copy(json_string_value(json_object_get(obj, "name")));
  dim->description =
    str_copy(json_string_value(json_object_get(obj, "description")));
  return 0;
}

/* Assigns DLiteProperty from a json object */
int dlite_json_entity_prop(const json_t *obj, size_t ndim,
                           const DLiteDimension *d, DLiteProperty *prop)
{
  const char *ptype;
  json_t *dims;
  json_t *item;
  size_t i, size;
  UNUSED(d);

  if (!json_is_object(obj)) return 1;
  prop->name = str_copy(json_string_value(json_object_get(obj, "name")));
  ptype = json_string_value(json_object_get(obj, "type"));
  dlite_type_set_dtype_and_size(ptype, &(prop->type), &(prop->size));
  dims = json_object_get(obj, "dims");
  size = json_array_size(dims);
  assert(ndim == size);
  prop->ndims = (int)size;
  if (prop->ndims > 0) {
    for(i = 0; i < size; i++) {
      const char *s;
      item = json_array_get(dims, i);
      if (!(s = json_string_value(item)))
        return err(1, "property dimensions should be strings");
      prop->dims[i] = strdup(s);
    }
  }
  prop->unit =
    str_copy(json_string_value(json_object_get(obj, "unit")));
  prop->iri =
    str_copy(json_string_value(json_object_get(obj, "iri")));
  prop->description =
    str_copy(json_string_value(json_object_get(obj, "description")));
  return 0;
}

/* Creates a DLiteMeta from a json object */
DLiteMeta *dlite_json_entity(json_t *obj)
{
  DLiteMeta *entity = NULL;
  char *uri=NULL;
  char *iri=NULL;
  char *desc=NULL;
  int i, j, nprop, ndim;
  json_t *jd;
  json_t *jp;
  json_t *item;
  DLiteDimension *dims=NULL;
  DLiteProperty *props=NULL;

  if (json_is_object(obj)) {
      uri = dlite_json_uri(obj);
      if (uri) {
        ndim = dlite_json_entity_dim_count(obj);
        nprop = dlite_json_entity_prop_count(obj);

        if ((nprop < 0) || (ndim < 0)) {
          err(0, "errors in the definition of the entity %s", uri);
        }
        else if (nprop == 0) {
          err(0, "no property for the entity %s", uri);
        }
        else if ((nprop > 0) && (ndim >= 0)) {
          item = json_object_get(obj, "iri");
          if (item) iri = str_copy(json_string_value(item));

          item = json_object_get(obj, "description");
          if (item) desc = str_copy(json_string_value(item));

          if (ndim > 0) {
            dims = calloc(ndim, sizeof(DLiteDimension));
            jd = json_object_get(obj, "dimensions");
            assert(json_array_size(jd) == (size_t)ndim);
            for (i = 0; i < ndim; i++) {
              item = json_array_get(jd, i);
              dlite_json_entity_dim(item, dims + i);
            }
          }

          props = calloc(nprop, sizeof(DLiteProperty));
          jp = json_object_get(obj, "properties");
          assert(json_array_size(jp) == (size_t)nprop);
          for (i = 0; i < nprop; i++) {
            item = json_array_get(jp, i);
            dlite_json_entity_prop(item, ndim, dims, props + i);
          }

          entity = dlite_meta_create(uri, iri, desc,
                                     ndim, dims,
                                     nprop, props);
        }
      }
      else
        err(0, "name, version, and namespace must be given.");
  }
  if (uri) free(uri);
  if (desc) free(desc);
  if (dims) {
    for (i=0; i<ndim; i++) {
      free(dims[i].name);
      free(dims[i].description);
    }
    free(dims);
  }
  if (props) {
    for (i=0; i<nprop; i++) {
      if (props[i].name) free(props[i].name);
      for (j=0; j<props[i].ndims; j++)
        if (props[i].dims[j]) free(props[i].dims[j]);
      if (props[i].dims) free(props[i].dims);
      if (props[i].description) free(props[i].description);
    }
    free(props);
  }
  return entity;
}


static DLiteStoragePlugin dlite_json_plugin = {
  /* head */
  "json",                          /* name */
  NULL,                            /* freeapi */

  /* basic api */
  dlite_json_open,                 /* open */
  dlite_json_close,                /* close */

  /* queue api */
  NULL,                            /* iterCreate */
  NULL,                            /* iterNext */
  NULL,                            /* iterFree */
  dlite_json_get_uuids,            /* getUUIDs */

  /* direct api */
  NULL,                            /* loadInstance */
  NULL,                            /* saveInstance */

  /* datamodel api */
  dlite_json_datamodel,            /* dataModel */
  dlite_json_datamodel_free,       /* dataModelFree */

  dlite_json_get_metadata,         /* getMetaURI */
  dlite_json_resolve_dimensions,   /* resolveDimensions */
  dlite_json_get_dimension_size,   /* getDimensionSize */
  dlite_json_get_property,         /* getProperty */

  /* -- datamodel api (optional) */
  dlite_json_set_metadata,         /* setMetaURI */
  dlite_json_set_dimension_size,   /* setDimensionSize */
  dlite_json_set_property,         /* setProperty */

  dlite_json_has_dimension,        /* hasDimension */
  dlite_json_has_property,         /* hasProperty */

  dlite_json_get_dataname,         /* getDataName, obsolute */
  dlite_json_set_dataname,         /* setDataName, obsolute */

  /* internal data */
  NULL                             /* data */
};


DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(int *iter)
{
  UNUSED(iter);
  return &dlite_json_plugin;
}
