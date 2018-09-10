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
#include "err.h"

#include "dlite-utils.h"

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


/* Error macros for when DLiteDataModel instance d in available */
#define DFAIL0(d, msg) \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid); goto fail;} while (0)

#define DFAIL1(d, msg, a1) \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid, a1); goto fail;} while (0)

#define DFAIL2(d, msg, a1, a2) \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid, a1, a2); goto fail;} while (0)

#define DFAIL3(d, msg, a1, a2, a3) \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid, a1, a2, a3); goto fail;} while (0)

#define DFAIL4(d, msg, a1, a2, a3, a4)                            \
  do {err(-1, "%s/%s: " msg, d->s->uri, d->uuid, a1, a2, a3, a4); \
    goto fail;} while (0)



/* Storage for json backend. */
typedef struct {
  DLiteStorage_HEAD
  json_t *root;       /* json root object */
  int compact;        /* whether to write output in compact format */
} DLiteJsonStorage;


/* How the json-data is organised */
typedef enum {
  fmtNormal,  /* normal data */
  fmtEntity,  /* entitity */
  fmtSchema   /* entity_schema */
} DataFormat;

/* Data model for json backend. */
typedef struct {
  DLiteDataModel_HEAD
  json_t *instance;     /* json object to instance, borrowed reference */
  json_t *meta;         /* json object to metadata, borrowed reference */
  json_t *dimensions;   /* json object to dimensions, borrowed reference */
  json_t *properties;   /* json object to properties, borrowed reference */
  DataFormat fmt;       /* */
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

  Valid \a options are:

    - rw   Read and write: open existing file or create new file (default)
    - r    Read-only: open existing file for read-only
    - a    Append: open existing file for read and write
    - w    Write: truncate existing file or create new file
    - c    Whether to write output in compact format

 */
DLiteStorage *dlite_json_open(const char *uri, const char *options)
{
  DLiteJsonStorage *s;
  DLiteStorage *retval=NULL;
  json_error_t error;
  size_t n;

  if (!(s = calloc(1, sizeof(DLiteJsonStorage)))) FAIL0("allocation failure");

  if (!options || !options[0] || strcmp(options, "rw") == 0) { /* default */
    s->root = json_load_file(uri, 0, &error);
    s->writable = 1;
  } else if (strcmp(options, "r") == 0) {
    s->root = json_load_file(uri, 0, &error);
    s->writable = 0;
  } else if (strcmp(options, "a") == 0) {
    s->root = json_load_file(uri, 0, &error);
    s->writable = 1;
  } else if (strcmp(options, "w") == 0) {
    s->root = json_object();
    s->writable = 1;
  } else if (strchr(options, 'c')) {
    s->compact = 1;
  } else {
    FAIL1("invalid options '%s', must be 'rw' (read and write), "
          "'r' (read-only), 'w' (write) or 'a' (append)", options);
  }

  if (s->root == NULL) {
    n = strlen(error.text);
    if (n > 0)
      printf("JSON parse error on line %d: %s\n", error.line, error.text);
    else
      printf("JSON parse error on line %d\n", error.line);
    FAIL2("cannot open: '%s' with options '%s'", uri, options);
  }
  if (!json_is_object(s->root))
    FAIL1("expected an object as root in json file: '%s'", uri);

  retval = (DLiteStorage *)s;

 fail:
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
    json_dump_file(storage->root, storage->uri, flags);
  }
  json_decref(storage->root);
  return nerr;
}

DLiteDataModel *dlite_json_datamodel(const DLiteStorage *s, const char *id)
{
  DLiteJsonDataModel *d=NULL;
  DLiteDataModel *retval=NULL;
  DLiteJsonStorage *storage = (DLiteJsonStorage *)s;
  //const char *name, *version, *namespace;
  char uuid[DLITE_UUID_LENGTH + 1];
  json_t *data;

  if (!(d = calloc(1, sizeof(DLiteJsonDataModel))))
    FAIL0("allocation failure");

  if (id && dlite_get_uuid(uuid, id) < 0) goto fail;

  if (id && (data = json_object_get(storage->root, uuid))) {
    /* Instance `id` exists - assign datamodel... */
    if (!json_is_object(data))
      FAIL2("expected a json object for instance '%s' in '%s'",
            id, storage->uri);
    d->instance = data;
    d->meta = json_object_get(data, "meta");
    d->dimensions = json_object_get(data, "dimensions");
    d->properties = json_object_get(data, "properties");

  } else if (json_object_get(storage->root, "namespace") &&
             json_object_get(storage->root, "version") &&
             json_object_get(storage->root, "name")) {
    /* Instance is a metadata definition */
    data = storage->root;
    d->instance = data;
    if (!(d->meta = json_object_get(data, "meta"))) d->fmt = fmtEntity;
    d->dimensions = json_object_get(data, "dimensions");
    d->properties = json_object_get(data, "properties");

  } else if (json_object_get(storage->root, "schema_namespace") &&
             json_object_get(storage->root, "schema_version") &&
             json_object_get(storage->root, "schema_name")) {
    /* Instance is a meta-metadata definition (schema) */
    data = storage->root;
    d->instance = data;
    if (!(d->meta = json_object_get(data, "meta"))) d->fmt = fmtSchema;
    d->dimensions = json_object_get(data, "schema_dimensions");
    d->properties = json_object_get(data, "schema_properties");

  } else {
    /* Instance `uuid` does not exists - create new instance and
       assign the datamodel... */
    if (!storage->writable)
      FAIL2("cannot create new instance '%s' in read-only storage %s",
            uuid, storage->uri);
    d->instance = json_object();
    json_object_set_new(storage->root, uuid, d->instance);
    d->meta = json_object();
    json_object_set_new(d->instance, "meta", d->meta);
    d->dimensions = json_object();
    json_object_set_new(d->instance, "dimensions", d->dimensions);
    d->properties = json_object();
    json_object_set_new(d->instance, "properties", d->properties);
  }

  retval = (DLiteDataModel *)d;
 fail:
  if (!retval && d) free(d);
  return retval;
}

/**
  Closes data handle dh5. Returns non-zero on error.
 */
int dlite_json_datamodel_free(DLiteDataModel *d)
{
  UNUSED(d);
  /* be carefull to not free memory owned by root... */
  //DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  //if (data->meta) json_decref(data->meta);
  //if (data->dimensions) json_decref(data->dimensions);
  //if (data->properties) json_decref(data->properties);
  //if (data->instance) json_decref(data->instance);
  return 0;
}


/**
  Returns pointer to (malloc'ed) metadata or NULL on error.
 */
char *dlite_json_get_metadata(const DLiteDataModel *d)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  const char *name=NULL;
  const char *version=NULL;
  const char *space=NULL;

  if (!data->meta) {
    if (data->fmt == fmtEntity)
      return strdup(DLITE_SCHEMA_ENTITY);
    else if (data->fmt == fmtEntity)
      return strdup(DLITE_SCHEMA_ENTITY);
    else
      return err(1, "unexpected json format number %d", data->fmt), NULL;
  }

  name = object_get_string(data->meta, "name");
  version = object_get_string(data->meta, "version");
  space = object_get_string(data->meta, "namespace");

  return dlite_join_meta_uri(name, version, space);
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

  case fmtEntity:
  case fmtSchema:
    if (strcmp(name, "ndimensions") == 0)
      return json_array_size(data->dimensions);
    else if (strcmp(name, "nproperties") == 0)
      return json_array_size(data->properties);
    //else if (strcmp(name, "nrelations") == 0)
    //  return json_array_size(data->relations);
    else
      return err(-1, "expedted metadata dimension names are 'ndimensions', "
                 "'nproperties' or 'nrelations'; got '%s'", name);
  }
  assert(0);  /* never reached */
}


/* Recursive help function for handling n-dimensioal arrays */
static int getdim(size_t d, const json_t *arr, void **pptr,
                  DLiteType type, size_t size,
                  size_t ndims, const size_t *dims,
                  json_t *jroot)
{
  size_t i;
  if (d < ndims) {
    if (json_array_size(arr) != dims[d])
      return errx(1, "length of dimension %lu is %lu, expected %lu",
                  d, json_array_size(arr), dims[d]);
    for (i=0; i<dims[d]; i++) {
      const json_t *a = json_array_get(arr, i);
      if (getdim(d+1, a, pptr, type, size, ndims, dims, jroot)) return 1;
    }
  } else {
    if (dlite_json_get_value(*pptr, arr, type, size, jroot)) return 1;
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
  case fmtEntity:
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

  if (dlite_split_meta_uri(metadata, &name, &version, &namespace))
    return 1;

  object_set_string(data->meta, "name", name);
  object_set_string(data->meta, "version", version);
  object_set_string(data->meta, "namespace", namespace);

  free(name);
  free(version);
  free(namespace);
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
                      size_t ndims, const size_t *dims)
{
  int i;
  json_t *item;
  if (d < ndims) {
    json_t *arr = json_array();
    for (i=0; i<(int)dims[d]; i++) {
      if (!(item = setdim(d + 1, pptr, type, size, ndims, dims))) return NULL;
      json_array_append_new(arr, item);
    }
    return arr;
  }  else {
    item = dlite_json_set_value(*pptr, type, size);
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
    if (!(item = setdim(0, (void **)&ptr, type, size, ndims, dims))) return 1;
    json_object_set_new(datamodel->properties, name, item);
  } else {
    if (!(item = dlite_json_set_value(ptr, type, size))) return 1;
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

  n = json_object_size(storage->root);
  if (n > 0) {
    if (!(names = malloc((n + 1)*sizeof(char *))))
      ok = 0;
    else {
      iter = json_object_iter(storage->root);
      i = 0;
      while(iter)
      {
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
  int n = 0;

  if (json_is_object(obj)) {

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
  size_t i, j, size;

  if (!json_is_object(obj)) return 1;
  prop->name = str_copy(json_string_value(json_object_get(obj, "name")));
  prop->description =
    str_copy(json_string_value(json_object_get(obj, "description")));
  ptype = json_string_value(json_object_get(obj, "type"));
  dlite_type_set_dtype_and_size(ptype, &(prop->type), &(prop->size));
  dims = json_object_get(obj, "dims");
  size = json_array_size(dims);
  prop->ndims = (int)size;
  if (prop->ndims > 0) {
    prop->dims = calloc(prop->ndims, sizeof(int));
    for(i = 0; i < size; i++) {
      item = json_array_get(dims, i);
      for(j = 0; j < ndim; j++) {
        if (str_equal(json_string_value(item), d[j].name)) {
          prop->dims[i] = j;
          break;
        }
      }
    }
  }
  return 0;
}

/* Creates a DLiteEntity from a json object */
DLiteEntity *dlite_json_entity(json_t *obj)
{
  DLiteEntity *entity = NULL;
  char *uri=NULL;
  char *desc=NULL;
  int i, nprop, ndim;
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

          entity = dlite_entity_create(uri, desc, ndim, dims, nprop, props);
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
      if (props[i].dims) free(props[i].dims);
      if (props[i].description) free(props[i].description);
    }
    free(props);
  }
  return entity;
}

/* Create an entity from a json storage and the given entity ID */
DLiteEntity *dlite_json_get_entity(const DLiteStorage *s, const char *id)
{
  DLiteJsonStorage *storage = (DLiteJsonStorage *)s;
  char *uri=NULL;
  json_t *obj=NULL;

  if (id && *id) {
    /* id given - find it in the json storage */
    char uuid[DLITE_UUID_LENGTH+1], suuid[DLITE_UUID_LENGTH+1];
    if (dlite_get_uuid(uuid, id) < 0) goto fail;

    /* If root is an array, we set the root to the matching item */
    if (json_is_array(storage->root)) {
      size_t i, size;
      size = json_array_size(storage->root);
      for(i=0; i < size; i++) {
        json_t *item = json_array_get(storage->root, i);
        uri = dlite_json_uri(item);
        if (dlite_get_uuid(suuid, uri) < 0) goto fail;
        if (str_equal(suuid, uuid)) {
          obj = item;
          break;
        }
      }
    } else if (json_is_object(storage->root)) {
      if (!(uri = dlite_json_uri(storage->root)))
        FAIL1("cannot find valid entiry in storage '%s'", s->uri);
      if (dlite_get_uuid(suuid, uri) < 0)
        goto fail;
      if (str_equal(suuid, uuid))
        obj = storage->root;

      if (!obj)
        FAIL2("cannot find entity with id '%s' in storage '%s'", id, s->uri);
    }

  } else {
    /* no id - check if only one entity is defined in the json storage */
    if (json_is_array(storage->root)) {
      size_t size = json_array_size(storage->root);
      if (size == 1) {
        json_t *item = json_array_get(storage->root, 0);
        if ((uri = dlite_json_uri(item)))
          obj = item;
      } else {
        FAIL2("storage '%s' is an array of %lu items, but only one entity "
              "is expected when no id is provided", s->uri, size);
      }
    } else if (json_is_object(storage->root)) {
      if ((uri = dlite_json_uri(storage->root)))
        obj = storage->root;
    }

    if (!obj)
      FAIL1("cannot find valid entity in storage '%s'", s->uri);
  }

 fail:
  if (uri) free(uri);
  return (obj) ? dlite_json_entity(obj) : NULL;
}


int dlite_json_set_entity(DLiteStorage *s, const DLiteEntity *e)
{
  return dlite_entity_save(s, e);
}


DLitePlugin dlite_json_plugin = {
  "json",

  dlite_json_open,
  dlite_json_close,

  dlite_json_datamodel,
  dlite_json_datamodel_free,

  dlite_json_get_metadata,
  dlite_json_get_dimension_size,
  dlite_json_get_property,

  /* optional */
  dlite_json_get_uuids,

  dlite_json_set_metadata,
  dlite_json_set_dimension_size,
  dlite_json_set_property,

  dlite_json_has_dimension,
  dlite_json_has_property,

  dlite_json_get_dataname,
  dlite_json_set_dataname,

  dlite_json_get_entity,
  dlite_json_set_entity
};
