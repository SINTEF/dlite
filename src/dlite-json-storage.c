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
} DLiteJsonStorage;


/* Data model for json backend. */
typedef struct {
  DLiteDataModel_HEAD
  json_t *instance;     /* json object to instance */
  json_t *meta;         /* json object to metadata */
  json_t *dimensions;   /* json object to dimensions */
  json_t *properties;   /* json object to properties */
} DLiteJsonDataModel;


char *object_get_string(json_t *obj, const char *key)
{
  char *retval=NULL;
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
  else
    value = json_string(val);
  json_object_set(obj, key, value);
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
  else
    value = json_integer(val);
  json_object_set(obj, key, value);
}

void object_set_real(json_t *obj, const char *key, const double val)
{
  json_t *value = json_object_get(obj, key);  
  if (json_is_real(value))
    json_real_set(value, val);
  else
    value = json_real(val);
  json_object_set(obj, key, value);
}

/**
  Returns an url to the metadata.

  Valid \a options are:

    rw   Read and write: open existing file or create new file (default)
    r    Read-only: open existing file for read-only
    a    Append: open existing file for read and write
    w    Write: truncate existing file or create new file

 */
DLiteStorage *dlite_json_open(const char *uri, const char *options)
{
  DLiteJsonStorage *s;
  DLiteStorage *retval=NULL;
  json_error_t error;

  printf("dlite_json_open %s %s", uri, options);

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
  } else {
    FAIL1("invalid options '%s', must be 'rw' (read and write), "
          "'r' (read-only), 'w' (write) or 'a' (append)", options);
  }

  if (s->root == NULL)
    FAIL2("cannot open: '%s' with options '%s'", uri, options);
  if (!json_is_object(s->root))
    FAIL1("expected an object as root in json file: '%s'", uri);

  retval = (DLiteStorage *)s;

 fail:
  if (!retval && s) free(s);
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
    json_dump_file(storage->root, storage->uri, 0);
  }
  json_decref(storage->root);
  return nerr;
}

DLiteDataModel *dlite_json_datamodel(const DLiteStorage *s, const char *uuid)
{
  DLiteJsonDataModel *d;
  DLiteDataModel *retval=NULL;
  DLiteJsonStorage *storage = (DLiteJsonStorage *)s;
  json_t *data = json_object_get(storage->root, uuid);

  if (!(d = calloc(1, sizeof(DLiteJsonDataModel)))) FAIL0("allocation failure");

  if (data == NULL) {
    d->instance = json_object();
    json_object_set(storage->root, uuid, d->instance);
    d->meta = json_object();
    json_object_set(d->instance, "meta", d->meta);
    d->dimensions = json_object();
    json_object_set(d->instance, "dimensions", d->dimensions);
    d->properties = json_object();
    json_object_set(d->instance, "properties", d->properties);    
  }
  else if (json_is_object(data)) {
    d->instance = data;
    d->meta = json_object_get(data, "meta");
    d->dimensions = json_object_get(data, "dimensions");
    d->properties = json_object_get(data, "properties");
  }
  else
    FAIL2("expected a json object for instance '%s' in '%s'", uuid, storage->uri);

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
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  int nerr=0;
  json_decref(data->meta);
  json_decref(data->dimensions);
  json_decref(data->properties);  
  json_decref(data->instance);
  return nerr;
}


/**
  Returns pointer to (malloc'ed) metadata or NULL on error.
 */
const char *dlite_json_get_metadata(const DLiteDataModel *d)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  char *name=NULL;
  char *version=NULL;
  char *namespace=NULL;

  name = object_get_string(data->meta, "name");
  version = object_get_string(data->meta, "version");
  namespace = object_get_string(data->meta, "namespace");

  return dlite_join_meta_uri(name, version, namespace);
}

/**
  Returns the size of dimension `name` or -1 on error.
 */
int dlite_json_get_dimension_size(const DLiteDataModel *d, const char *name)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  return object_get_integer(data->dimensions, name);
}

/**
  Copies property `name` to memory pointed to by `ptr`.
  Returns non-zero on error.
 */
int dlite_json_get_property(const DLiteDataModel *d, const char *name, void *ptr,
                            DLiteType type, size_t size, int ndims, const int *dims)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  json_t *value = json_object_get(data->properties, name);
  json_data_t *jd = json_get_data(value);
  if (jd) {
    if ((type == dliteInt) && (jd->dtype == 'i')) {

    }
    else if ((type == dliteFloat) && (jd->dtype == 'r')) {

    }
    else if ((type == dliteBool) && (jd->dtype == 'b')) {

    }
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
int dlite_json_set_dimension_size(DLiteDataModel *d, const char *name, int size)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  object_set_integer(data->dimensions, name, size);
  return 0;
}

/**
  Sets property `name` to the memory (of `size` bytes) pointed to by `ptr`.
  Returns non-zero on error.
*/
int dlite_json_set_property(DLiteDataModel *d, const char *name, const void *ptr,
                            DLiteType type, size_t size, int ndims, const int *dims)
{
  DLiteJsonDataModel *data = (DLiteJsonDataModel *)d;
  int *i;
  double *r;

  switch (type) {

  case dliteBlob:
    return 1;

  case dliteInt:
    i = (int*)(ptr);
    object_set_integer(data->properties, name, *i);
    return 0;

  case dliteBool:
    return 1;

  case dliteUInt:
    return 1;

  case dliteFloat:
    r = (double*)(ptr);
    object_set_real(data->properties, name, *r);
    return 0;

  case dliteFixString:
    return 1;

  case dliteStringPtr:
    return 1;

  default:
    return errx(-1, "Invalid type number: %d", type);
  }

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

  n = json_object_size(storage->root);
  if (n > 0) {
    names = malloc((n + 1)*sizeof(char*));
    if (!(names = malloc((n + 1)*sizeof(char *)))) FAIL0("allocation failure");

    iter = json_object_iter(storage->root);
    i = 0;
    while(iter)
    {
      key = json_object_iter_key(iter);
      len = strlen(key) + 1;
      if (!(names[i] = malloc(len))) FAIL0("allocation failure");
      memcpy(names[i], key, len);
      i++;
    }
    names[n] = NULL;
  }

fail:
  if (names) {
    for(i=0; i < n; i++)
      if (names[i])
        free(names[i]);
    free(names);
  }
  names = NULL;

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
  return object_get_string(data->instance, "dataname");
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


char *dlite_json_uri(json_t * obj)
{
  char *uri;
  char *name=NULL;
  char *version=NULL;
  char *namespace=NULL;
  int n = 0; 

  if (json_is_object(obj)) {
    name = object_get_string(obj, "name");
    version = object_get_string(obj, "version");
    namespace = object_get_string(obj, "namespace");

    name = json_object_get(obj, "name");
    if (!str_is_whitespace(json_string_value(name)))
      n++;
    version = json_object_get(obj, "version");
    if (!str_is_whitespace(json_string_value(version)))
      n++;
    namespace = json_object_get(obj, "namesapce");
    if (!str_is_whitespace(json_string_value(namespace)))
      n++;

    if (n == 3)
      return dlite_join_meta_uri(name, version, namespace);
  }
  return NULL;
}

/* Create DLiteProperty from a json object */
DLiteDimension *dlite_json_entity_dim(json_t *obj)
{
  DLiteDimension *dim = NULL;
  if (json_is_object(obj)) {
    dim = malloc(sizeof(DLiteDimension));
    dim->name = str_copy(json_string_value(json_object_get(obj, "name")));
    dim->description = str_copy(json_string_value(json_object_get(obj, "description")));
  }
  return dim;
}

/* Create DLiteProperty from a json object */
DLiteProperty *dlite_json_entity_prop(json_t *obj, size_t ndim, DLiteDimension **d)
{
  DLiteProperty *prop = NULL;
  char *ptype;
  json_t *dims;
  json_t *item;
  size_t i, j, size;

  if (json_is_object(obj)) {
    prop = malloc(sizeof(DLiteProperty));
    prop->name = str_copy(json_string_value(json_object_get(obj, "name")));
    prop->description = str_copy(json_string_value(json_object_get(obj, "description")));
    ptype = json_string_value(json_object_get(obj, "type"));    
    dlite_type_set_dtype_and_size(ptype, &(prop->type), &(prop->size));
    dims = json_object_get(obj, "dims");
    size = json_array_size(dims);
    prop->ndims = (int)size;
    if (prop->ndims > 0) {
      prop->dims = malloc(sizeof(int) * prop->ndims);
      for(i = 0; i < size; i++) {
        item = json_array_get(dims, i);
        for(j = 0; j < ndim; j++) {
          if (str_equal(json_string_value(item), d[j]->name)) {
            prop->dims[i] = j;
            break;
          }
        }
      }
    }
  }
  return prop;
}

/* Create DLiteEntity from a json object */
DLiteEntity *dlite_json_entity(json_t *obj)
{
  DLiteEntity *entity = NULL;
  char *uri;
  char *desc;
  int nprop, ndim;
  size_t i, size;
  json_t *jd;
  json_t *jp;
  DLiteDimension **dims;
  DLiteProperty **props;

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
          desc = str_copy(json_string_value(json_object_get(obj, "description")));

          if (ndim > 0) {
            dims = malloc(ndim * sizeof(DLiteDimension *));
            jd = json_object_get(obj, "dimensions");
            size = json_array_size(jd);
            for (i = 0; i < size; i++)
              dims[i] = dlite_json_entity_dim(json_array_get(dims, i));            
          }

          props = malloc(nprop * sizeof(DLiteProperty *));
          jp = json_object_get(obj, "properties");
          size = json_array_size(jp);
          for (i = 0; i < size; i++)
            props[i] = dlite_json_entity_prop(json_array_get(props, i), ndim, dims);

          //entity = dlite_entity_create(uri, desc, ndim, dims, nprop, props);
        }
      }
      else
        err(0, "name, version, and namespace must be given.");
  }
  return entity;
}

/* Create an entity from a json storage and the given entity ID */
DLiteEntity *dlite_json_get_entity(const DLiteStorage *s, const char *uuid)
{
  DLiteJsonStorage *storage = (DLiteJsonStorage *)s;
  json_t *obj = NULL;
  json_t *item = NULL;
  size_t i, size;
  char *uri=NULL;

  // Find the uuid in the json storage
  if (uuid) {
    if (json_is_object(storage->root)) {
      uri = dlite_json_uri(storage->root);
      if (uri) {
        if (str_equal(uuid, uri))
          obj = storage->root;
        //?str_free(uri);
      }
      else
        obj = json_object_get(storage->root, uuid);
    }
    else if (json_is_array(storage->root)) {
      size = json_array_size(storage->root);
      for(i = 0; i < size; i++) {
        item = json_array_get(storage->root, i);
        uri = dlite_json_uri(item);
        if (str_equal(uuid, uri))
          obj = item;
        //?str_free(uri);
        if (obj)
          break;
      }
    }
  }
  // No uuid given, check if only one entity is defined in the json storage
  else {
    uri = dlite_json_uri(storage->root);
    if (uri)
      obj = storage->root;
    else if (json_is_array(storage->root)) {
      size = json_array_size(storage->root);
      if (size > 0) {
        item = json_array_get(storage->root, 0);
        uri = dlite_json_uri(item);
        if (uri)
          obj = item;
      }
    }
  }

  return dlite_json_entity(obj);
}

int dlite_json_set_entity(DLiteStorage *s, const DLiteEntity *e)
{
  DLiteJsonStorage *storage = (DLiteJsonStorage *)s;
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

