#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "utils/err.h"
#include "utils/compat.h"
#include "utils/strutils.h"
#define JSMN_HEADER
#include "utils/jsmn.h"

#include "dlite.h"
#include "dlite-macros.h"


/* Expands to `a - b` if `a > b` else to `0`. */
#define PDIFF(a, b) (((size_t)(a) > (size_t)(b)) ? (a) - (b) : 0)

/* These macros could have been simplified with __VA_ARGS__, but not
   all compilers support that... */
#define PRINT(fmt)                                      \
  do {                                                  \
    int m = snprintf(dest+n, PDIFF(size, n), fmt);      \
    if (m < 0) goto fail;                               \
    n += m;                                             \
  } while (0)
#define PRINT1(fmt, a1)                                 \
  do {                                                  \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1);  \
    if (m < 0) goto fail;                               \
    n += m;                                             \
  } while (0)
#define PRINT2(fmt, a1, a2)                                     \
  do {                                                          \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1, a2);      \
    if (m < 0) goto fail;                                       \
    n += m;                                                     \
  } while (0)
#define PRINT3(fmt, a1, a2, a3)                                 \
  do {                                                          \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1, a2, a3);  \
    if (m < 0) goto fail;                                       \
    n += m;                                                     \
  } while (0)
#define PRINT4(fmt, a1, a2, a3, a4)                                     \
  do {                                                                  \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1, a2, a3, a4);      \
    if (m < 0) goto fail;                                               \
    n += m;                                                             \
  } while (0)


/* Iterator struct */
struct _DLiteJsonIter {
  const char *src;        /*!< Source document to search */
  jsmntok_t *tokens;      /*!< Allocated tokens */
  unsigned int ntokens;   /*!< Number of allocated tokens */
  const jsmntok_t *t;     /*!< Pointer to current token */
  unsigned int n;         /*!< Current token number */
  unsigned int size;      /*!< Size of the root object */
  char metauuid[DLITE_UUID_LENGTH+1];  /*!< UUID of metadata */
};


/*
  Serialise instance `inst` to `dest`, formatted as JSON.

  No more than `size` bytes are written to `dest` (incl. the
  terminating NUL).

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `size`, the number of bytes that would
  have been written if `size` was large enough is returned.  On error, a
  negative value is returned.
*/
int dlite_json_sprint(char *dest, size_t size, DLiteInstance *inst,
                      int indent, DLiteJsonFlag flags)
{
  DLiteTypeFlag f = (DLiteTypeFlag)flags;
  int n=0, ok=0, m, j;
  size_t i;
  char *in = malloc(indent + 1);
  memset(in, ' ', indent);
  in[indent] = '\0';

  PRINT1("%s{\n", in);
  if (flags & dliteJsonUuid)
    PRINT2("%s  \"uuid\": \"%s\",\n", in, inst->uuid);
  if (inst->uri)
    PRINT2("%s  \"uri\": \"%s\",\n", in, inst->uri);
  PRINT2("%s  \"meta\": \"%s\",\n", in, inst->meta->uri);

  if ((flags & dliteJsonMetaAsData) || dlite_instance_is_data(inst)) {
    /* Standard format */
    PRINT1("%s  \"dimensions\": {\n", in);
    for (i=0; i < inst->meta->_ndimensions; i++) {
      char *name = inst->meta->_dimensions[i].name;
      int val = DLITE_DIM(inst, i);
      char *c = (i < inst->meta->_ndimensions - 1) ? "," : "";
      PRINT4("%s    \"%s\": %d%s\n", in, name, val, c);
    }
    PRINT1("%s  },\n", in);

    PRINT1("%s  \"properties\": {\n", in);
    for (i=0; i < inst->meta->_nproperties; i++) {
      char *c = (i < inst->meta->_nproperties - 1) ? "," : "";
      DLiteProperty *p = inst->meta->_properties + i;
      void *ptr = dlite_instance_get_property_by_index(inst, i);
      size_t *dims = DLITE_PROP_DIMS(inst, i);
      PRINT2("%s    \"%s\": ", in, p->name);
      m = dlite_property_print(dest+n, PDIFF(size, n), ptr, p, dims, 0, -2, f);
      if (m < 0) return -1;
      n += m;
      PRINT1("%s\n", c);
    }
    PRINT1("%s  }\n", in);

  } else {
    /* Special format for entities */
    void *ptr;
    DLiteMeta *met = (DLiteMeta *)inst;
    if ((ptr = dlite_instance_get_property(inst, "description")))
      PRINT2("%s  \"description\": \"%s\",\n", in, *(char **)ptr);

    PRINT1("%s  \"dimensions\": [\n", in);
    for (i=0; i < met->_ndimensions; i++) {
      char *c = (i < met->_ndimensions - 1) ? "," : "";
      DLiteDimension *d = met->_dimensions + i;
      PRINT1("%s    {\n", in);
      PRINT2("%s      \"name\": \"%s\"", in, d->name);
      if (d->description)
        PRINT2(",\n%s      \"description\": \"%s\"\n", in, d->description);
      PRINT2("%s    }%s\n", in, c);
    }
    PRINT1("%s  ],\n", in);

    PRINT1("%s  \"properties\": [\n", in);
    for (i=0; i < met->_nproperties; i++) {
      char typename[32];
      char *c = (i < met->_nproperties - 1) ? "," : "";
      DLiteProperty *p = met->_properties + i;
      dlite_type_set_typename(p->type, p->size, typename, sizeof(typename));
      PRINT1("%s    {\n", in);
      PRINT2("%s      \"name\": \"%s\",\n", in, p->name);
      PRINT2("%s      \"type\": \"%s\"", in, typename);
      if (p->ndims) {
        PRINT1(",\n%s      \"dims\": [", in);
        for (j=0; j < p->ndims; j++) {
          char *cc = (j < p->ndims - 1) ? ", " : "";
          PRINT2("\"%s\"%s", p->dims[j], cc);
        }
        PRINT("]");
      }
      if (p->unit)
        PRINT2(",\n%s      \"unit\": \"%s\"", in, p->unit);
      if (p->description)
        PRINT2(",\n%s      \"description\": \"%s\"", in, p->description);
      PRINT2("\n%s    }%s\n", in, c);
    }
    PRINT1("%s  ],\n", in);
  }

  PRINT1("%s}", in);

  ok = 1;
 fail:
  free(in);
  return (ok) ? n : -1;
}


/*
  Like dlite_json_sprint(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*size`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
*/
int dlite_json_asprint(char **dest, size_t *size, size_t pos,
                       DLiteInstance *inst, int indent, DLiteJsonFlag flags)
{
  int m;
  void *q;
  size_t newsize;
  if (!dest && !*dest) *size = 0;
  m = dlite_json_sprint(*dest + pos, PDIFF(*size, pos), inst, indent, flags);
  if (m < 0) return m;
  if (m < (int)PDIFF(*size, pos)) return m;

  /* Reallocate buffer to required size. */
  newsize = m + pos + 1;
  if (!(q = realloc(*dest, newsize))) return -1;
  *dest = q;
  *size = newsize;
  m = dlite_json_sprint(*dest + pos, PDIFF(*size, pos), inst, indent, flags);
  assert(0 <= m && m < (int)*size);
  return m;
}


/*
  Like dlite_json_sprint(), but returns allocated buffer with
  serialised instance.
*/
char *dlite_json_aprint(DLiteInstance *inst, int indent, DLiteJsonFlag flags)
{
  char *dest=NULL;
  size_t size=0;
  if ((dlite_json_asprint(&dest, &size, 0, inst, indent, flags)) < 0)
    return NULL;
  return dest;
}

/*
  Like dlite_json_sprint(), but prints to stream `fp`.

  Returns number or bytes printed or a negative number on error.
*/
int dlite_json_fprint(FILE *fp, DLiteInstance *inst, int indent,
                      DLiteJsonFlag flags)
{
  int m;
  char *buf=NULL;
  size_t size=0;
  if ((m = dlite_json_asprint(&buf, &size, 0, inst, indent, flags)) >= 0) {
    fprintf(fp, "%s\n", buf);
    free(buf);
  }
  return m;
}

/*
  Prints json representation of `inst` to standard output.

  Returns number or bytes printed or a negative number on error.
*/
int dlite_json_print(DLiteInstance *inst)
{
  return dlite_json_fprint(stdout, inst, 0, 0);
}


/* ================================================================
 * Scanning
 * ================================================================ */

/* Writes the UUID of the instance represented by `obj` to `uuid`.

   Returns non-zero on error. */
static int get_meta_uuid(char *uuid, const char *src, const jsmntok_t *obj)
{
  jsmntok_t *item;
  int retval = 1;
  char *buf=NULL;
  size_t size=0;
  const char *s = src + obj->start;
  int len = obj->end - obj->start;
  if (!(item = jsmn_item(src, (jsmntok_t *)obj, "meta")))
    FAIL2("no 'meta' in json repr. of instance: %.*s", len, s);

  if (item->type == JSMN_OBJECT) {
    int n=0;
    jsmntok_t *t;
    if (!(t = jsmn_item(src, item, "namespace")))
      FAIL2("no \"namespace\" in meta for object %.*s", len, s);
    n += strnput(&buf, &size, n, src + t->start, t->end - t->start);

    if (!(t = jsmn_item(src, item, "version")))
      FAIL2("no \"version\" in meta for object %.*s", len, s);
    n += asnpprintf(&buf, &size, n, "/%.*s", t->end - t->start, src+t->start);

    if (!(t = jsmn_item(src, item, "name")))
      FAIL2("no \"name\" in meta for object %.*s", len, s);
    n += asnpprintf(&buf, &size, n, "/%.*s", t->end - t->start, src+t->start);

  } else if (item->type == JSMN_STRING) {
    strnput(&buf, &size, 0, src + item->start, item->end - item->start);

  } else
    return err(1, "\"meta\" in json repr. of instance should be either an "
               "object or a string: %.*s", len, s);

  if (dlite_get_uuid(uuid, buf) < 0) goto fail;

  retval = 0;
 fail:
  if (buf) free(buf);
  return retval;
}

/*
  Search for instances in the JSON document provided to dlite_json_iter_init()
  and returns a pointer to the corresponding jsmn token.

  Returns NULL if there are no more tokens left.

  See also dlite_json_next().
*/
static const jsmntok_t *nexttok(DLiteJsonIter *iter, int *length)
{
  while (iter->n < iter->size) {
    const jsmntok_t *t = iter->t;
    if (length) *length = t->end - t->start;
    iter->t += jsmn_count(t + 1) + 2;
    iter->n++;
    if (*iter->metauuid) {
      char uuid[DLITE_UUID_LENGTH+1];
      const jsmntok_t *v = t + 1;
      get_meta_uuid(uuid, iter->src, v);
      if (strncmp(uuid, iter->metauuid, DLITE_UUID_LENGTH) == 0) return t;
    } else
      return t;
  }
  return NULL;
}


/*
  Help function for parsing an instance.
  - src: json source
  - obj: jsmn representation of the json object to parse
  - id:  id of `obj`

  Returns new `instance` or NULL on error.
*/
static DLiteInstance *parse_instance(const char *src, jsmntok_t *obj,
                                     const char *id)
{
  int ok=0;
  jsmntok_t *item, *t;
  char *buf=NULL, *uri=NULL, uuid[DLITE_UUID_LENGTH+1];
  size_t i, size=0, *dims=NULL;
  DLiteInstance *inst=NULL;
  const DLiteMeta *meta;
  jsmntype_t dimtype = 0;
  char *name=NULL, *version=NULL, *namespace=NULL;

  assert(obj->type == JSMN_OBJECT);

  /* Get uri */
  if ((item = jsmn_item(src, obj, "uri"))) {
    uri = strndup(src + item->start, item->end - item->start);
    if (!id) id = uri;
    if (dlite_get_uuid(uuid, uri) < 0) goto fail;
  } else if ((item = jsmn_item(src, obj, "uuid"))) {
    if (item->end - item->start != DLITE_UUID_LENGTH)
      FAIL2("invalid length of \"uuid\": %.*s",
            item->end - item->start, src + item->start);
    strncpy(uuid, src + item->start, sizeof(uuid));
    if (!id) id = uuid;
  }

  /* Get metadata */
  if ((item = jsmn_item(src, obj, "meta"))) {
    int len = item->end - item->start;
    if (item->type == JSMN_STRING) {
      strnput(&buf, &size, 0, src + item->start, len);
    } else if (item->type == JSMN_OBJECT) {
      int n=0;
      if (!(t = jsmn_item(src, item, "namespace")))
        FAIL1("no \"namespace\" in meta for object %s", id);
      n += strnput(&buf, &size, n, src + t->start, t->end - t->start);

      if (!(t = jsmn_item(src, item, "version")))
        FAIL1("no \"version\" in meta for object %s", id);
      n += asnpprintf(&buf, &size, n, "/%.*s", t->end - t->start, src+t->start);

      if (!(t = jsmn_item(src, item, "name")))
        FAIL1("no \"name\" in meta for object %s", id);
      n += asnpprintf(&buf, &size, n, "/%.*s", t->end - t->start, src+t->start);
    } else {
      FAIL1("\"meta\" not string or object in object %s", id);
    }
    if (!(meta = dlite_meta_get(buf)) &&
        !(meta = (DLiteMeta *)dlite_json_sscan(src, buf, NULL)))
      FAIL2("cannot find metadata '%s' when loading '%s' - please add the "
            "right storage to DLITE_STORAGES and try again", buf, id);
  } else {
    /* If "meta" is not given, we assume it is an entity */
    meta = dlite_get_entity_schema();
  }
  assert(meta);

  /* Allocate dimensions */
  if (!(dims = calloc(meta->_ndimensions, sizeof(size_t))))
    FAIL("allocation failure");

  /* Parse dimensions */
  if (meta->_ndimensions > 0) {
    if (!(item = jsmn_item(src, obj, "dimensions")))
      FAIL1("no \"dimensions\" in object %s", id);
    dimtype = item->type;

    if (item->type == JSMN_OBJECT) {
      if (item->size != (int)meta->_ndimensions)
        FAIL3("expected %d dimensions, got %d in instance %s",
              (int)meta->_ndimensions, item->size, id);
      for (i=0; i < meta->_ndimensions; i++) {
        DLiteDimension *d = meta->_dimensions + i;
        if (!(t = jsmn_item(src, item, d->name)))
          FAIL2("missing dimension \"%s\" in %s", d->name, id);
        if (t->type != JSMN_PRIMITIVE)
          FAIL3("value '%.*s' of dimension should be an integer: %s",
                t->end-t->start, src+t->start, id);
        dims[i] = atoi(src + t->start);
      }
    } else if (item->type == JSMN_ARRAY) {
      int n=0;
      if (!dlite_meta_is_metameta(meta))
        FAIL1("only metadata can have array dimensions: %s", id);
      if (meta->_ndimensions >= 2) dims[n++] = item->size;
      if ((t = jsmn_item(src, obj, "properties")))
        dims[n++] = t->size;
      if (meta->_ndimensions >= 3 && (t = jsmn_item(src, obj, "relations")))
        dims[n++] = t->size;
    } else {
      FAIL1("\"dimensions\" must be object or array: %s", id);
    }
  }

  printf("------------------ meta -------------------------\n");
  dlite_json_print((DLiteInstance *)meta);
  printf("-------------------------------------------------\n");
  printf("*** id='%s'\n", id);
  printf("*** dims:");
  {
    size_t i;
    for (i=0; i < meta->_ndimensions; i++)
      printf(" %d", (int)dims[i]);
    printf("\n");
  }


  /* Create instance */
  if (!(inst = dlite_instance_create(meta, dims, id))) goto fail;

  printf("*** inst = %p\n", (void *)inst);
  printf("------------------ inst -------------------------\n");
  dlite_json_print(inst);
  printf("-------------------------------------------------\n");


  /* Parse properties */
  if (meta->_nproperties > 0) {
    jsmntok_t *base=obj;  // assigned to make cppcheck happy...
    if (!(item = jsmn_item(src, obj, "properties")))
      FAIL1("no \"properties\" in object %s", id);
    if (dimtype && item->type != dimtype)
      FAIL1("\"properties\" must have same type as \"dimensions\": %s", id);

    /* -- `base` is the base object to read properties from */
    if (item->type == JSMN_OBJECT)
      base = item;
    else if (item->type == JSMN_ARRAY)
      base = obj;
    else
      FAIL1("\"properties\" must be object or array: %s", id);
    assert(base->type == JSMN_OBJECT);

    /* -- infer name, version and namespace */
    if ((t = jsmn_item(src, base, "uri"))) {
      if (t->type != JSMN_STRING) FAIL1("uri must be a string: %s", id);
      strnput(&buf, &size, 0, src + t->start, t->end - t->start);
      if (dlite_split_meta_uri(buf, &name, &version, &namespace))
        FAIL2("invalid uri '%s' in %s", buf, id);
      if (!inst->uri) inst->uri = strdup(buf);
    } else {
      int status;

    ErrTry:
      status = dlite_split_meta_uri(id, &name, &version, &namespace);
    ErrCatch(1):
      break;
      ErrEnd;

      if (status && dlite_instance_is_meta(inst))
        FAIL1("cannot infer name, version and namespace from id: %s", id);
    }

    /* -- read properties */
    for (i=0; i < meta->_nproperties; i++) {
      void *ptr;
      DLiteProperty *p = meta->_properties + i;
      size_t *pdims = DLITE_PROP_DIMS(inst, i);
      if (!(ptr = dlite_instance_get_property_by_index(inst, i))) goto fail;
      if ((t = jsmn_item(src, base, p->name))) {
        strnput(&buf, &size, 0, src+t->start, t->end-t->start);
        if (dlite_property_scan(buf, ptr, p, pdims, 0) < 0) goto fail;
      } else {
        /* -- if not given, use inferred name, version and namespace */
        if (strcmp(p->name, "name") == 0) {
          if (dlite_property_scan(name, ptr, p, pdims, 0) < 0) goto fail;
        } else if (strcmp(p->name, "version") == 0) {
          if (dlite_property_scan(version, ptr, p, pdims, 0) < 0) goto fail;
        } else if (strcmp(p->name, "namespace") == 0) {
          if (dlite_property_scan(namespace, ptr, p, pdims, 0) < 0) goto fail;
        } else
          FAIL2("missing property \"%s\" in %s", p->name, id);
      }

      printf("*** load prop: %s : %d\n", meta->uri, (int)i);
      if (meta->_loadprop) {
        printf("+++\n");
        meta->_loadprop(inst, i);
      }
    }
  }
  if (dlite_instance_is_meta(inst)) dlite_meta_init((DLiteMeta *)inst);

  ok = 1;
 fail:
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  if (dims) free(dims);
  if (buf) free(buf);
  if (uri) free(uri);
  if (!ok && inst) {
    dlite_instance_decref(inst);
    inst = NULL;
  }
  return inst;
}


/*
  Returns a new instance scanned from `src`.

  `id` is the uri or uuid of the instance to load.  If the string only
  contain one instance (of the required metadata), `id` may be NULL.

  If `metaid` is not NULL, it should be the URI or UUID of the
  metadata of the returned instance.  It is an error if no such
  instance exists in the source.

  Returns the instance or NULL on error.
*/
DLiteInstance *dlite_json_sscan(const char *src, const char *id,
                                const char *metaid)
{
  int i, r;
  char *buf=NULL;
  DLiteJsonIter *iter=NULL;
  DLiteInstance *inst=NULL;
  unsigned int ntokens=0;
  jsmntok_t *tokens=NULL, *root;
  jsmn_parser parser;
  size_t srclen = strlen(src);

  errno = 0;

  jsmn_init(&parser);
  r = jsmn_parse_alloc(&parser, src, srclen, &tokens, &ntokens);
  if (r < 0) FAIL1("error parsing json: %s", jsmn_strerror(r));
  root = tokens;
  if (root->type != JSMN_OBJECT) FAIL("json root should be an object");

  if (!id) {
    if (jsmn_item(src, root, "properties")) {
      inst = parse_instance(src, root, id);
    } else {
      int len;
      if (!(iter = dlite_json_iter_init(src, srclen, metaid))) goto fail;
      const jsmntok_t *t1 = nexttok(iter, &len);
      const jsmntok_t *t2 = nexttok(iter, NULL);
      if (!t1) {
        if (metaid)
          FAIL1("json source has no instance with meta id: '%s'", metaid);
        else
          FAIL("no instances in json source");
      }
      if (t2) FAIL("`id` is required when scanning json input with "
                     "multiple instances");
      jsmntok_t *val = (jsmntok_t *)t1 + 1;
      buf = strndup(src + t1->start, t1->end - t1->start);
      if (!(inst = parse_instance(src, val, buf))) goto fail;
    }
  } else {
    int n=1;
    char uuid[DLITE_UUID_LENGTH+1];
    if (dlite_get_uuid(uuid, id) < 0) goto fail;
    for (i=0; i < root->size; i++) {
      char uuid2[DLITE_UUID_LENGTH+1];
      jsmntok_t *key = root + n;
      jsmntok_t *val = root + n+1;
      int len = key->end - key->start;
      if (key->type != JSMN_STRING) FAIL("expect json keys to be strings");
      if (len >= (int)sizeof(buf))
        FAIL3("key exceeded maximum key length (%d): %.*s",
              (int)sizeof(buf), len, src+key->start);
      buf = strndup(src+key->start, len);
      if (dlite_get_uuid(uuid2, buf) < 0) goto fail;
      if (strcmp(uuid2, uuid) == 0) {
        if (!(inst = parse_instance(src, val, id))) goto fail;
        break;
      }
      n += jsmn_count(val) + 2;
    }
  }

  assert(inst);
  if (metaid) {
    char uuid[DLITE_UUID_LENGTH + 1];
    if (dlite_get_uuid(uuid, metaid) < 0 || strcmp(metaid, uuid) != 0) {
      if (!id) id = (inst->iri) ? inst->iri : inst->uuid;
      err(1, "instance '%s' has meta id '%s' but '%s' is expected",
          id, inst->meta->uri, metaid);
      dlite_instance_decref(inst);
      inst = NULL;
    }
  }

 fail:
  free(tokens);
  if (buf) free(buf);
  if (iter) dlite_json_iter_deinit(iter);

  return inst;
}


/*
  Like dlite_json_sscan(), but scans instance `id` from stream `fp` instead
  of a string.

  Returns the instance or NULL on error.
*/
DLiteInstance *dlite_json_fscan(FILE *fp, const char *id, const char *metaid)
{
  DLiteInstance *inst;
  char *buf;
  if (!(buf = fu_readfile(fp))) return NULL;
  inst = dlite_json_sscan(buf, id, metaid);
  free(buf);
  return inst;
}






/*
  Creates and returns a new iterator used by dlite_json_next().

  Arguments
  - src: input JSON string to search.
  - length: length of `src`.  If zero or negative, all of `src` will be used.
  - metaid: limit the search to instances of metadata with this id.

  The source should be a JSON object with keys being instance UUIDs
  and values being the JSON representation of the individual instances.

  Returns new iterator or NULL on error.
*/
DLiteJsonIter *dlite_json_iter_init(const char *src, int length,
                                    const char *metaid)
{
  int r, ok=0;
  DLiteJsonIter *iter=NULL;
  jsmn_parser parser;

  if (!(iter = calloc(1, sizeof(DLiteJsonIter)))) FAIL("allocation failure");

  if (length <= 0) length = strlen(src);
  jsmn_init(&parser);
  r = jsmn_parse_alloc(&parser, src, length, &iter->tokens, &iter->ntokens);
  if (r < 0) FAIL1("error parsing json: %s", jsmn_strerror(r));
  if (iter->tokens->type != JSMN_OBJECT) FAIL("json root should be an object");
  iter->src = src;
  iter->t = iter->tokens + 1;
  iter->size = iter->tokens->size;
  if (metaid && dlite_get_uuid(iter->metauuid, metaid) < 0) goto fail;

  ok=1;
 fail:
  if (!ok) dlite_json_iter_deinit(iter);
  return iter;
}

/*
  Free's iterator created with dlite_json_iter_init().
*/
void dlite_json_iter_deinit(DLiteJsonIter *iter)
{
  if (iter) {
    if (iter->tokens) free(iter->tokens);
    free(iter);
  }
}



/*
  Search for instances in the JSON document provided to dlite_json_iter_init()
  and returns a pointer to instance UUIDs.

  `iter` should be an iterator created with dlite_json_iter_init().

  If `length` is given, it is set to the length of the returned identifier.

  Returns a pointer to the next matching UUID or NULL if there are no more
  matches left.
*/
const char *dlite_json_next(DLiteJsonIter *iter, int *length)
{
  const jsmntok_t *t = nexttok(iter, length);
  if (t) printf("*** next: %.*s\n", t->end - t->start, iter->src + t->start);

  if (t) return iter->src + t->start;
  return NULL;
}
