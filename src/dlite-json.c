#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "utils/err.h"
#include "utils/compat.h"
#include "utils/strutils.h"

#include "getuuid.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-json.h"


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


/* Forward declarations */
static const jsmntok_t *nexttok(DLiteJsonIter *iter, int *length);


/*
  Help function for serialise instance `inst` to `dest`, formatted as JSON.

  No more than `size` bytes are written to `dest` (incl. the
  terminating NUL).

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `size`, the number of bytes that would
  have been written if `size` was large enough is returned.  On error, a
  negative value is returned.
*/
int _dlite_json_sprint(char *dest, size_t size, const DLiteInstance *inst,
                       int indent, DLiteJsonFlag flags)
{
  int n=0, ok=0, m, j;
  size_t i;
  char *in = malloc(indent + 1);
  char *prop_comma = (inst->_parent && !(flags & dliteJsonNoParent)) ? "," : "";
  DLiteTypeFlag f = dliteFlagQuoted;
  if (flags & dliteJsonCompactRel) f |= dliteFlagCompactRel;
  memset(in, ' ', indent);
  in[indent] = '\0';

  PRINT1("%s{\n", in);
  if (inst->uri)
    PRINT2("%s  \"uri\": \"%s\",\n", in, inst->uri);
  if (flags & dliteJsonWithUuid)
    PRINT2("%s  \"uuid\": \"%s\",\n", in, inst->uuid);
  if (flags & dliteJsonWithMeta ||
      dlite_instance_is_data(inst) ||
      dlite_instance_is_metameta(inst))
    PRINT2("%s  \"meta\": \"%s\",\n", in, inst->meta->uri);

  if (dlite_instance_is_data(inst)) {  // data
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
      size_t *shape= DLITE_PROP_DIMS(inst, i);
      PRINT2("%s    \"%s\": ", in, p->name);
      m = dlite_property_print(dest+n, PDIFF(size, n), ptr, p, shape, 0, -2, f);
      if (m < 0) return -1;
      n += m;
      PRINT1("%s\n", c);
    }
    PRINT2("%s  }%s\n", in, prop_comma);

  } else if (flags & dliteJsonArrays) {  // metadata: soft5 format
    DLiteMeta *met = (DLiteMeta *)inst;
    char *description =
      *((char **)dlite_instance_get_property(inst, "description"));
    if (description)
      PRINT2("%s  \"description\": \"%s\",\n", in, description);

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
      if (p->ref)
        PRINT2(",\n%s      \"$ref\": \"%s\"", in, p->ref);
      if (p->ndims) {
        PRINT1(",\n%s      \"dims\": [", in);
        for (j=0; j < p->ndims; j++) {
          char *cc = (j < p->ndims - 1) ? ", " : "";
          PRINT2("\"%s\"%s", p->shape[j], cc);
        }
        PRINT("]");
      }
      if (p->unit && *p->unit)
        PRINT2(",\n%s      \"unit\": \"%s\"", in, p->unit);
      if (p->description && *p->description)
        PRINT2(",\n%s      \"description\": \"%s\"", in, p->description);
      PRINT2("\n%s    }%s\n", in, c);
    }
    PRINT1("%s  ]\n", in);

    if (dlite_instance_get_property((DLiteInstance *)inst->meta, "relations")) {
      PRINT1("%s  \"relations\": [\n", in);
      for (i=0; i < met->_nrelations; i++) {
        int m;
        DLiteRelation *r = met->_relations + i;
        PRINT1("%s    [", in);
        m = strquote(dest+n, PDIFF(size, n), r->s);
        if (m < 0) goto fail;
        n += m;
        PRINT(", ");
        m = strquote(dest+n, PDIFF(size, n), r->p);
        if (m < 0) goto fail;
        n += m;
        PRINT(", ");
        m = strquote(dest+n, PDIFF(size, n), r->o);
        if (m < 0) goto fail;
        n += m;
        PRINT("]\n");

      }
      PRINT2("%s  ]%s\n", in, prop_comma);
    }

  } else {  // metadata: soft7 format
    DLiteMeta *met = (DLiteMeta *)inst;
    char *description =
      *((char **)dlite_instance_get_property(inst, "description"));

    if (description)
      PRINT2("%s  \"description\": \"%s\",\n", in, description);

    PRINT1("%s  \"dimensions\": {\n", in);
    for (i=0; i < met->_ndimensions; i++) {
      char *c = (i < met->_ndimensions - 1) ? "," : "";
      DLiteDimension *d = met->_dimensions + i;
      PRINT4("%s    \"%s\": \"%s\"%s\n", in, d->name, d->description, c);
    }
    PRINT1("%s  },\n", in);

    PRINT1("%s  \"properties\": {\n", in);
    for (i=0; i < met->_nproperties; i++) {
      char typename[32];
      char *c = (i < met->_nproperties - 1) ? "," : "";
      DLiteProperty *p = met->_properties + i;
      dlite_type_set_typename(p->type, p->size, typename, sizeof(typename));
      PRINT2("%s    \"%s\": {\n", in, p->name);
      PRINT2("%s      \"type\": \"%s\"", in, typename);
      if (p->ref)
        PRINT2(",\n%s      \"$ref\": \"%s\"", in, p->ref);
      if (p->ndims) {
        PRINT1(",\n%s      \"shape\": [", in);
        for (j=0; j < p->ndims; j++) {
          char *cc = (j < p->ndims - 1) ? ", " : "";
          PRINT2("\"%s\"%s", p->shape[j], cc);
        }
        PRINT("]");
      }
      if (p->unit && *p->unit)
        PRINT2(",\n%s      \"unit\": \"%s\"", in, p->unit);
      if (p->description && *p->description)
        PRINT2(",\n%s      \"description\": \"%s\"", in, p->description);
      PRINT2("\n%s    }%s\n", in, c);
    }
    PRINT1("%s  }\n", in);

    if (dlite_instance_get_property((DLiteInstance *)inst->meta, "relations")) {
      PRINT1("%s  \"relations\": [\n", in);
      for (i=0; i < met->_nrelations; i++) {
        int m;
        DLiteRelation *r = met->_relations + i;
        PRINT1("%s    [", in);
        m = strquote(dest+n, PDIFF(size, n), r->s);
        if (m < 0) goto fail;
        n += m;
        PRINT(", ");
        m = strquote(dest+n, PDIFF(size, n), r->p);
        if (m < 0) goto fail;
        n += m;
        PRINT(", ");
        m = strquote(dest+n, PDIFF(size, n), r->o);
        if (m < 0) goto fail;
        n += m;
        PRINT("]\n");
      }
      PRINT2("%s  ]%s\n", in, prop_comma);
    }
  }

  if (inst->_parent && !(flags & dliteJsonNoParent)) {
    char hex[DLITE_HASH_SIZE*2 + 1];
    if (strhex_encode(hex, sizeof(hex),
                      inst->_parent->hash, DLITE_HASH_SIZE) < 0)
      FAILCODE1(dliteValueError,
                "cannot encode hash of parent: %s", inst->_parent->uuid);
    PRINT1("%s  \"parent\": {\n", in);
    PRINT2("%s    \"uuid\": \"%s\",\n", in, inst->_parent->uuid);
    PRINT2("%s    \"hash\": \"%s\"\n", in, hex);
    PRINT1("%s  }\n", in);
  }

  PRINT1("%s}", in);

  ok = 1;
 fail:
  free(in);
  return (ok) ? n : -1;
}


/*
  Serialise instance `inst` to `dest`, formatted as JSON.

  No more than `size` bytes are written to `dest` (incl. the
  terminating NUL).

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `size`, the number of bytes that would
  have been written if `size` was large enough is returned.  On error, a
  negative value is returned.
*/
int dlite_json_sprint(char *dest, size_t size, const DLiteInstance *inst,
                      int indent, DLiteJsonFlag flags)
{
  char *in=NULL;
  int m, n=0;
  if (flags & dliteJsonSingle) {
    n = _dlite_json_sprint(dest, size, inst, indent, flags);
  } else {
    if (!(in = malloc(indent + 1))) FAIL("allocation failure");
    memset(in, ' ', indent);
    in[indent] = '\0';
    PRINT1("%s{\n", in);
    PRINT2("%s  \"%s\":", in,
           (flags & dliteJsonUriKey && inst->uri) ? inst->uri : inst->uuid);
    if ((m = _dlite_json_sprint(dest+n, PDIFF(size, n), inst, indent+2, flags)) < 0)
      goto fail;
    n += m;
    PRINT1("\n%s}", in);
    free(in);
  }
  return n;
 fail:
  if (in) free(in);
  return -1;
}


/*
  Like dlite_json_sprint(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*size`. Bytes at position less than `pos` are not changed.

  If `*dest` is NULL or `*size` is less than needed, `*dest` is
  reallocated and `*size` updated to the new buffer size.

  If `pos` is larger than `*size` the bytes at index `i` are
  initialized to space (' '), where ``*size <= i < pos``.

  Returns number or bytes written (not including terminating NUL) or a
  negative number on error.
*/
int dlite_json_asprint(char **dest, size_t *size, size_t pos,
                       const DLiteInstance *inst, int indent,
                       DLiteJsonFlag flags)
{
  int m;
  char *q;
  size_t newsize;

  if (!dest || !*dest || !*size) {
    /* Just count number of bytes to write */
    m = dlite_json_sprint(*dest, 0, inst, indent, flags);
    if (m < 0) return m;
  } else {
    /* Try to write to existing buffer */
    m = dlite_json_sprint(*dest + pos, PDIFF(*size, pos), inst, indent, flags);
    if (m < (int)PDIFF(*size, pos)) return m;
  }

  /* Reallocate buffer to required size. */

  // FIXME: newsize should really be `newsize = m + pos + 1;`.
  // If `inst` is a collection with relations, then
  // dlite_json_sprint() seems to report one byte too little when
  // called with size=0.
  //
  // Update: Added dedicated test for collection to test_json.c.
  // That test is not able to reproduce this error.
  newsize = m + pos + 2;
  if (!(q = realloc(*dest, newsize)))
    return err(dliteMemoryError, "allocation failure");

  /* Fill bytes from *size to pos with space */
  if (pos > *size) memset(q + *size, ' ', pos - *size);

  /* Update `*dest` and `*size` */
  *dest = q;
  *size = newsize;

  /* Write */
  m = dlite_json_sprint(q + pos, PDIFF(newsize, pos), inst, indent, flags);
  if (m < 0) return m;
  assert(m+pos < newsize);

  return m;
}


/*
  Like dlite_json_sprint(), but returns allocated buffer with
  serialised instance.
*/
char *dlite_json_aprint(const DLiteInstance *inst, int indent,
                        DLiteJsonFlag flags)
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
int dlite_json_fprint(FILE *fp, const DLiteInstance *inst, int indent,
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
int dlite_json_print(const DLiteInstance *inst)
{
  return dlite_json_fprint(stdout, inst, 0, 0);
}

/*
  Like dlite_json_sprint(), but prints the output to file `filename`.

  Returns number or bytes printed or a negative number on error.
 */
int dlite_json_printfile(const char *filename, const DLiteInstance *inst,
                         DLiteJsonFlag flags)
{
  FILE *fp;
  int m;
  if (!(fp = fopen(filename, "wt")))
    return err(1, "cannot write json to \"%s\"", filename);
  m = dlite_json_fprint(fp, inst, 0, flags);
  fclose(fp);
  return m;
}

/*
  Appends json representation of `inst` to json string pointed to by `*s`.

  On input, `*s` should be a malloc'ed string representation of a json object.
  It will be reallocated as needed.

  `*size` if the allocated size of `*s`.  It will be updated when `*s`
  is realocated.

  Returns number or bytes inserted or a negative number on error.
*/
int dlite_json_append(char **s, size_t *size, const DLiteInstance *inst,
                      DLiteJsonFlag flags)
{
  int r, n=0, retval=-1;
  unsigned int ntokens=0;
  jsmn_parser parser;
  jsmntok_t *tokens=NULL;
  size_t pos;

  errno = 0;
  jsmn_init(&parser);
  if ((r = jsmn_parse_alloc(&parser, *s, *size, &tokens, &ntokens)) < 0)
    FAIL1("error parsing json: %s", jsmn_strerror(r));
  if (r == 0) FAIL("cannot append to empty json string");
  if (tokens[0].type != JSMN_OBJECT)
    FAIL("can only append to json object");

  pos = tokens[0].end - 1;
  assert(pos > 0);
  /* Remove trailing spaces after the last item for prettier output.
     Also accept malformed json input with a trailing comma after the
     last item. */
  while (isspace((*s)[pos-1]) || (*s)[pos-1] == ',') pos--;

  if (tokens[0].size > 0 && (n = asnpprintf(s, size, pos, ",")) < 0)
    goto fail;
  pos += n;
  if ((n = asnpprintf(s, size, pos, "\n  \"%s\": ", inst->uuid)) < 0)
    goto fail;
  pos += n;
  if ((n = dlite_json_asprint(s, size, pos, inst, 2, flags)) < 0) goto fail;
  pos += n;
  if ((n = asnpprintf(s, size, pos, "\n}\n")) < 0) goto fail;
  pos += n;
  retval = pos - tokens[0].end;
 fail:
  free(tokens);
  return retval;
}


/* ================================================================
 * Scanning
 * ================================================================ */

/* Returns a malloc buffer with the URI of `obj` or NULL on error. */
static char *get_uri(const char *src, const jsmntok_t *obj)
{
  const jsmntok_t *t, *t1, *t2, *t3;
  if ((t = jsmn_item(src, obj, "uri")))
    return strndup(src + t->start, t->end - t->start);
  if ((t = jsmn_item(src, obj, "identity")))
    return strndup(src + t->start, t->end - t->start);
  if ((t1 = jsmn_item(src, obj, "name")) &&
      (t2 = jsmn_item(src, obj, "version")) &&
      (t3 = jsmn_item(src, obj, "namespace"))) {
    char *buf=NULL;
    asprintf(&buf, "%.*s/%.*s/%.*s",
             t3->end - t3->start, src + t3->start,
             t2->end - t2->start, src + t2->start,
             t1->end - t1->start, src + t1->start);
    return buf;
  }
  return NULL;
}

/* Returns a malloc buffer with the URI of the metadata of `obj` or
   NULL on error. */
static char *get_meta_uri(const char *src, const jsmntok_t *obj)
{
  const jsmntok_t *item;
  char *buf=NULL;
  size_t size=0;
  const char *s = src + obj->start;
  int len = obj->end - obj->start;
  if (!(item = jsmn_item(src, (jsmntok_t *)obj, "meta")))
    return strdup(DLITE_ENTITY_SCHEMA);  // default is entity schema

  if (item->type == JSMN_OBJECT) {
    if (!(buf = get_uri(src, item)))
      FAIL2("invalid meta for object %.*s", len, s);
  } else if (item->type == JSMN_STRING) {
    strnput(&buf, &size, 0, src + item->start, item->end - item->start);
  } else
    return err(1, "\"meta\" in json repr. of instance should be either an "
               "object or a string: %.*s", len, s), NULL;
 fail:
  return buf;
}

/* Writes the UUID of the instance represented by `obj` to `uuid`.
   Returns 0 on success, 1 if UUID is not found and -1 otherwise. */
static int get_uuid(char uuid[DLITE_UUID_LENGTH+1], const char *src,
                    const jsmntok_t *obj)
{
  const jsmntok_t *item;
  if (!(item = jsmn_item(src, (jsmntok_t *)obj, "uuid"))) return 1;
  if (item->end - item->start != DLITE_UUID_LENGTH)
    return err(dliteParseError, "UUID should have length %d, got %d",
               item->end - item->start, DLITE_UUID_LENGTH);
  if (dlite_get_uuidn(uuid, src+item->start, item->end-item->start) < 0)
    return -1;
  return 0;
}

/* Writes the UUID of the metadata of the instance represented by
   `obj` to `uuid`.  Returns non-zero on error. */
static int get_meta_uuid(char uuid[DLITE_UUID_LENGTH+1], const char *src,
                         const jsmntok_t *obj)
{
  int retval=1;
  char *buf;
  if (!(buf = get_meta_uri(src, obj))) goto fail;
  if (dlite_get_uuid(uuid, buf) < 0) goto fail;
  retval = 0;
 fail:
  free(buf);
  return retval;
}


/*
  Help function for parsing an instance.
  - src: json source
  - obj: jsmn representation of the json object to parse
  - id:  id of `obj`.  If NULL, it will be inferred.  Used for error reporting

  Returns new `instance` or NULL on error.
*/
static DLiteInstance *parse_instance(const char *src, jsmntok_t *obj,
                                     const char *id)
{
  int ok=0;
  const jsmntok_t *item, *t;
  char *buf=NULL, *uri=NULL, *metauri=NULL, uuid[DLITE_UUID_LENGTH+1];
  size_t i, *dims=NULL;
  DLiteInstance *inst=NULL;
  const DLiteMeta *meta=NULL;
  jsmntype_t dimtype = 0;
  char *name=NULL, *version=NULL, *namespace=NULL;

  assert(obj->type == JSMN_OBJECT);

  /* If instance already exists, return it immediately */
  if (id && dlite_instance_has(id, 0))
      return dlite_instance_get(id);

  /* Get uri and uuid */
  uri = get_uri(src, obj);
  if (!uri && (item = jsmn_item(src, obj, "uuid"))) {
    strncpy(uuid, src + item->start, DLITE_UUID_LENGTH);
    uuid[DLITE_UUID_LENGTH] = '\0';
  } else {
    if (dlite_get_uuid(uuid, (uri) ? uri : id) < 0) goto fail;
  }

  /* Check id */
  if (id && *id) {
    char uuid2[DLITE_UUID_LENGTH+1];
    if (dlite_get_uuid(uuid2, id) < 0) goto fail;
    if (strcmp(uuid, uuid2) != 0)
      FAIL3("instance has id \"%s\", expected \"%s\" (%s)", uuid, uuid2, id);
  }
  if (uri) id = uri;

  /* Get metadata */
  if ((item = jsmn_item(src, obj, "meta"))) {
    if (!(metauri = get_meta_uri(src, obj))) goto fail;
    if (!(meta = dlite_meta_get(metauri)))
      FAIL2("cannot find metadata '%s' when loading '%s' - please add the "
            "right storage to DLITE_STORAGES and try again", metauri, id);
  } else {
    /* If "meta" is not given, we assume it is an entity. */
    meta = dlite_get_entity_schema();  // borrowed reference
    dlite_meta_incref((DLiteMeta *)meta);
  }
  assert(meta);

  /* Allocate dimensions */
  if (!(dims = calloc(meta->_ndimensions, sizeof(size_t))))
    FAILCODE(dliteMemoryError, "allocation failure");

  /* Parse dimensions */
  if (dlite_meta_is_metameta(meta)) {
    /* For metadata, dimension sizes are inferred from the size of
       "dimensions", "properties" and "relations". */
    size_t n=0;
    if ((t = jsmn_item(src, obj, "dimensions"))) dims[n++] = t->size;
    if ((t = jsmn_item(src, obj, "properties"))) dims[n++] = t->size;
    if ((t = jsmn_item(src, obj, "relations")))  dims[n++] = t->size;
    if (n != meta->_ndimensions)
      FAILCODE1(dliteParseError, "metadata does not confirm to schema, "
                "please check dimensions, properties and/or relations: %s",
                id);

  } else {
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
          if (dlite_meta_is_metameta(meta)) {
            const jsmntok_t *tt;
            if (t->type != JSMN_STRING)
              FAIL1("expected dimension description, got: %.6s...",
                    src + t->start);
            if (d->name[0] != 'n')
              FAIL1("meta-metadata dimension names should start with \"n\": %s",
                    d->name);
            if (!(tt = jsmn_item(src, obj, d->name+1)))
              FAIL1("no metadata array named %s", d->name+1);
            dims[i] = tt->size;
          } else {
            if (t->type == JSMN_PRIMITIVE)
              dims[i] = atoi(src + t->start);
            else
              FAIL3("value '%.*s' of dimension should be an integer: %s",
                    t->end-t->start, src+t->start, id);
          }
        }
      } else {
        FAIL1("\"dimensions\" of data instances must be a json object: %s", id);
      }
    }
  }

  /* Create instance */
  if (!(inst = dlite_instance_create(meta, dims, id))) goto fail;

  /* Parse properties */
  if (meta->_nproperties > 0) {
    const jsmntok_t *base=obj;  // assigned to make cppcheck happy...
    if (!(item = jsmn_item(src, obj, "properties")))
      FAIL1("no \"properties\" in object %s", id);
    if (dimtype && item->type != dimtype)
      FAIL1("\"properties\" must have same type as \"dimensions\": %s", id);

    /* -- `base` is the base object to read properties from */
    base = (dlite_instance_is_data(inst)) ? item : obj;
    //if (item->type == JSMN_OBJECT)
    //  base = item;
    //else if (item->type == JSMN_ARRAY)
    //  base = obj;
    //else
    //  FAIL1("\"properties\" must be object or array: %s", id);
    assert(base->type == JSMN_OBJECT);

    /* -- infer name, version and namespace */
    if (dlite_instance_is_meta(inst)) {
      if (uri && dlite_split_meta_uri(uri, &name, &version, &namespace))
        FAIL1("cannot infer name, version and namespace from uri '%s'", uri);
      if (!name && id && dlite_split_meta_uri(id, &name, &version, &namespace))
        FAIL1("cannot infer name, version and namespace from id '%s'", id);
    }

    /* -- assign uri */
    if (!inst->uri) {
      char uuid2[DLITE_UUID_LENGTH+1];
      if (uri)
        inst->uri = strdup(uri);
      else if (id && dlite_get_uuid(uuid2, id) > 0)
        inst->uri = strdup(id);
    }

    /* -- read properties */
    for (i=0; i < meta->_nproperties; i++) {
      DLiteProperty *p = meta->_properties + i;
      size_t *pdims = DLITE_PROP_DIMS(inst, i);
      void *ptr = DLITE_PROP(inst, i);
      if (DLITE_PROP_NDIM(inst, i) > 0) ptr = *(void **)ptr;
      if ((t = jsmn_item(src, base, p->name))) {
        if (t->type == JSMN_ARRAY) {
          if (dlite_property_jscan(src, t, NULL, ptr, p, pdims, 0) < 0)
            goto fail;
        } else if (t->type == JSMN_OBJECT) {
          if (dlite_property_jscan(src, t, p->name, ptr, p, pdims, 0) < 0)
            goto fail;
        } else {
          if (!ptr)
            FAIL1("cannot assign property with NULL destination: %s", p->name);
          if (dlite_type_scan(src+t->start, t->end - t->start, ptr, p->type,
                              p->size, 0) < 0)
            goto fail;
        }
      } else if (dlite_instance_is_meta(inst)) {
        /* -- if not given, use inferred name, version and namespace */
        if (strcmp(p->name, "name") == 0) {
          if (dlite_property_scan(name, ptr, p, pdims, 0) < 0) goto fail;
        } else if (strcmp(p->name, "version") == 0) {
          if (dlite_property_scan(version, ptr, p, pdims, 0) < 0) goto fail;
        } else if (strcmp(p->name, "namespace") == 0) {
          if (dlite_property_scan(namespace, ptr, p, pdims, 0) < 0) goto fail;
        } else
          warnx("missing property \"%s\" in %s", p->name, id);
      } else
        FAIL2("missing property \"%s\" in %s", p->name, id);

      if (meta->_loadprop) meta->_loadprop(inst, i);
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
  if (metauri) free(metauri);
  if (!ok && inst) {
    dlite_instance_decref(inst);
    inst = NULL;
  }
  if (meta) dlite_meta_decref((DLiteMeta *)meta);
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

  if (jsmn_item(src, root, "properties")) {
    inst = parse_instance(src, root, id);
  } else if (!id || !*id) {
    int len;
    if (!(iter = dlite_json_iter_create(src, srclen, metaid))) goto fail;
    const jsmntok_t *t1 = nexttok(iter, &len);
    const jsmntok_t *t2 = nexttok(iter, NULL);
    if (!t1) {
      if (metaid)
        FAIL1("json source has no instance with meta id: '%s'", metaid);
      else
        FAIL("no instances in json source");
    }
    if (t2) FAIL("`id` (or `metaid`) is required when scanning json input "
                 "with multiple instances");
    jsmntok_t *val = (jsmntok_t *)t1 + 1;
    buf = strndup(src + t1->start, t1->end - t1->start);
    inst = parse_instance(src, val, buf);
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
      buf = strndup(src+key->start, len);
      if (dlite_get_uuid(uuid2, buf) < 0) goto fail;
      free(buf);
      buf = NULL;
      if (strcmp(uuid2, uuid) == 0) {
        if (!(inst = parse_instance(src, val, id))) goto fail;
        break;
      }
      n += jsmn_count(val) + 2;
    }
  }
  if (!inst) goto fail;

  if (metaid) {
    char uuid[DLITE_UUID_LENGTH + 1];
    if (dlite_get_uuid(uuid, metaid) < 0 ||
        (strcmp(metaid, uuid) != 0 && strcmp(metaid, inst->meta->uri) != 0)) {
      if (!id) id = inst->uuid;
      err(1, "instance '%s' has meta id '%s' but '%s' is expected",
          id, inst->meta->uri, metaid);
      dlite_instance_decref(inst);
      inst = NULL;
    }
  }

 fail:
  free(tokens);
  if (buf) free(buf);
  if (iter) dlite_json_iter_free(iter);

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
  Like dlite_json_sscan(), but scans instance `id` from file
  `filename` instead of a string.

  Returns the instance or NULL on error.
*/
DLiteInstance *dlite_json_scanfile(const char *filename, const char *id,
                                   const char *metaid)
{
  DLiteInstance *inst;
  FILE *fp;
  char *buf;
  if (!(fp = fopen(filename, "r")))
    FAIL1("cannot open storage \"%s\"", filename);
  if (!(buf = fu_readfile(fp))) goto fail;
  if (!(inst = dlite_json_sscan(buf, id, metaid))) {
    /* write full error message */
    char *msg=NULL;
    size_t size=0, n=0;
    n += asnpprintf(&msg, &size, n, "error loading instance ");
    if (id) n += asnpprintf(&msg, &size, n, "with id \"%s\" ", id);
    if (metaid) n += asnpprintf(&msg, &size, n, "of type \"%s\" ", metaid);
    n += asnpprintf(&msg, &size, n, "from file \"%s\"", filename);
    errx(1, "%s", msg);
    free(msg);
  }
 fail:
  if (fp) fclose(fp);
  if (buf) free(buf);
  return inst;
}


/* ================================================================
 * Checking
 * ================================================================ */

/*
  Check format of a parsed JSON string.

  `src` is the JSON string to check.

  `tokens` should be a parsed set of JSMN tokens corresponding to `src`.

  If `id` is not NULL, it is used to select what instance in a
  multi-entity formatted JSON string that will be used to assign
  flags.  If NULL, the first instance will be used.

  If `flags` is not NULL, the formatting (of the first entry in case
  of multi-entity format) will be investigated and `flags` set
  accordingly.

  Return the format of `src` or -1 on error.
 */
DLiteJsonFormat dlite_json_check(const char *src, const jsmntok_t *tokens,
                                 const char *id, DLiteJsonFlag *flags)
{
  DLiteJsonFormat retval=-1, fmt=-1;
  DLiteJsonFlag flg=0;
  const jsmntok_t *root, *key, *item, *props, *prop;
  char uuid[DLITE_UUID_LENGTH+1];

  root = tokens;
  if (root->type != JSMN_OBJECT) FAIL("json root should be an object");

  if (id && *id) {
    if (!(item = jsmn_item(src, root, id)))
      FAIL1("no such id in json source: \"%s\"", id);
  } else if (jsmn_item(src, root, "properties")) {
    item = root;
    flg |= dliteJsonSingle;
  } else if (root->size) {
    item = root + 2;
  } else {
    if (flags) *flags = 0;
    return dliteJsonDataFormat;  /* empty root object */
  }

  if (!(flg & dliteJsonSingle)) {
    int ver;
    key = item - 1;
    if ((ver = dlite_get_uuidn(uuid, src + key->start,
                               key->end - key->start)) < 0)
      FAIL2("cannot calculate uuid for key: \"%.*s\"",
            key->end - key->start, src + key->start);
    if (ver > 0) flg |= dliteJsonUriKey;
  }

  if (!(props = jsmn_item(src, item, "properties")))
    FAIL2("missing \"properties\" in json input \"%.*s\"",
          item->end - item->start, src + item->start);
  if (props->type == JSMN_ARRAY) {
    fmt = dliteJsonMetaFormat;
    flg |= dliteJsonArrays;
  } else if (props->type == JSMN_OBJECT) {
    prop = props + 1;
    assert(prop);
    if (prop->type == JSMN_OBJECT)
      fmt = (jsmn_item(src, item, "type")) ?
        dliteJsonMetaFormat : dliteJsonDataFormat;
    else
      fmt = dliteJsonDataFormat;
  } else {
    FAIL("properties must be an array or object");
  }

  if (jsmn_item(src, item, "uuid"))
    flg |= dliteJsonWithUuid;
  if (jsmn_item(src, item, "meta"))
    flg |= dliteJsonWithMeta;

  if (flags) *flags = flg;
  retval = fmt;
 fail:
  return retval;
}


/*
  Like dlite_json_check(), but checks string `src` with length `len`.

  Return the json format or -1 on error.
 */
DLiteJsonFormat dlite_json_scheck(const char *src, size_t len,
                                  const char *id, DLiteJsonFlag *flags)
{
  DLiteJsonFormat format=-1;
  jsmn_parser parser;
  jsmntok_t *tokens=NULL;
  unsigned int ntokens=0;
  int r;

  jsmn_init(&parser);
  r = jsmn_parse_alloc(&parser, src, len, &tokens, &ntokens);
  if (r < 0) FAIL1("error parsing json: %s", jsmn_strerror(r));

  format = dlite_json_check(src, tokens, id, flags);
 fail:
  if (tokens) free(tokens);
  return format;
}


/*
  Like dlite_json_scheck(), but checks the content of stream `fp` instead.

  Return the json format or -1 on error.
 */
DLiteJsonFormat dlite_json_fcheck(FILE *fp, const char *id,
                                  DLiteJsonFlag *flags)
{
  DLiteJsonFormat fmt;
  char *buf;
  if (!(buf = fu_readfile(fp))) return -1;
  fmt = dlite_json_scheck(buf, strlen(buf), id, flags);
  free(buf);
  return fmt;
}


/*
  Like dlite_json_scheck(), but checks the file `filename` instead.

  Return the json format or -1 on error.
 */
DLiteJsonFormat dlite_json_checkfile(const char *filename,
                                     const char *id, DLiteJsonFlag *flags)
{
  DLiteJsonFormat fmt=-1;
  FILE *fp=NULL;
  if (!(fp = fopen(filename, "r")))
    FAIL1("cannot open file \"%s\"", filename);
  if ((fmt = dlite_json_fcheck(fp, id, flags)) < 0)
    FAIL1("error checking json format of file \"%s\"", filename);
 fail:
  if (fp) fclose(fp);
  return fmt;
}




/* ================================================================
 * Iteration
 * ================================================================ */

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
  Search for instances in the JSON document provided to dlite_json_iter_create()
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
  Creates and returns a new iterator used by dlite_json_next().

  Arguments
  - src: input JSON string to search.
  - length: length of `src`.  If zero or negative, all of `src` will be used.
  - metaid: limit the search to instances of metadata with this id.

  The source should be a JSON object with keys being instance UUIDs
  and values being the JSON representation of the individual instances.

  Returns new iterator or NULL on error.
*/
DLiteJsonIter *dlite_json_iter_create(const char *src, int length,
                                      const char *metaid)
{
  int r;
  DLiteJsonIter *iter=NULL;
  jsmn_parser parser;

  if (!(iter = calloc(1, sizeof(DLiteJsonIter)))) FAILCODE(dliteMemoryError, "allocation failure");

  if (length <= 0) length = strlen(src);
  jsmn_init(&parser);
  r = jsmn_parse_alloc(&parser, src, length, &iter->tokens, &iter->ntokens);
  if (r < 0) FAIL1("error parsing json: %s", jsmn_strerror(r));
  if (r == 0) goto fail;
  if (iter->tokens->type != JSMN_OBJECT) FAIL("json root should be an object");
  iter->src = src;
  iter->t = iter->tokens + 1;
  iter->size = iter->tokens->size;
  if (metaid && dlite_get_uuid(iter->metauuid, metaid) < 0) goto fail;

  return iter;
 fail:
  if (iter) dlite_json_iter_free(iter);
  return NULL;
}

/*
  Free's iterator created with dlite_json_iter_create().
*/
void dlite_json_iter_free(DLiteJsonIter *iter)
{
  if (iter) {
    if (iter->tokens) free(iter->tokens);
    free(iter);
  }
}

/*
  Search for instances in the JSON document provided to dlite_json_iter_create()
  and returns a pointer to instance UUIDs.

  `iter` should be an iterator created with dlite_json_iter_create().

  If `length` is given, it is set to the length of the returned identifier.

  Returns a pointer to the next matching UUID or NULL if there are no more
  matches left.
*/
const char *dlite_json_next(DLiteJsonIter *iter, int *length)
{
  const jsmntok_t *t = nexttok(iter, length);
  if (t) return iter->src + t->start;
  return NULL;
}



/* ================================================================
 * JSON store
 * ================================================================ */

/* Iterater struct */
struct _DLiteJStoreIter {
  JStoreIter jiter;                    /*!< jstore iterater */
  char metauuid[DLITE_UUID_LENGTH+1];  /*!< UUID of metadata */
  jsmntok_t *tokens;                   /*!< pointer to allocated tokens */
  unsigned int ntokens;                /*!< number of allocated tokens */
};


/*
  Load content of json string `src` to json store `js`.
  `len` is the length of `src`.

  Returns json format or -1 on error.
 */
DLiteJsonFormat dlite_jstore_loads(JStore *js, const char *src, int len)
{
  jsmn_parser parser;
  jsmntok_t *tokens=NULL;
  unsigned int ntokens=0;
  char uuid[DLITE_UUID_LENGTH+1], *uri=NULL;
  int r;
  DLiteJsonFormat retval=-1, format;
  DLiteJsonFlag flags=0;
  char *dots = (len > 30) ? "..." : "";

  jsmn_init(&parser);
  if ((r = jsmn_parse_alloc(&parser, src, len, &tokens, &ntokens)) < 0)
    FAIL3("error parsing json string: \"%.30s%s\": %s",
          src, dots, jsmn_strerror(r));
  if (tokens->type != JSMN_OBJECT)
    FAIL2("root of json data must be an object: \"%.30s%s\"", src, dots);

  if ((format = dlite_json_check(src, tokens, NULL, &flags)) < 0) goto fail;

  if (flags & dliteJsonSingle) {
    uuid[0] = '\0';
    if (get_uuid(uuid, src, tokens) < 0) goto fail;
    if (!(uri = get_uri(src, tokens)) && !uuid[0])
      FAILCODE2(dliteParseError,
                "missing UUID and URI in json data: \"%.30s%s\"", src, dots);
    if (uri) {
      char uuid2[DLITE_UUID_LENGTH+1];
      if (dlite_get_uuid(uuid2, uri) < 0) goto fail;
      if (uuid[0] && strcmp(uuid, uuid2))
        FAILCODE2(dliteParseError,
                  "inconsistent URI and UUID in json data: uri=%s, uuid=%s",
                  uri, uuid);
      if (!uuid[0]) strncpy(uuid, uuid2, sizeof(uuid));
    }
    jstore_addn(js, uuid, DLITE_UUID_LENGTH, src, len);
  } else {
    jsmntok_t *t = tokens + 1;
    int i;
    for (i=0; i < tokens->size; i++) {
      jsmntok_t *v = t + 1;

      /* if `id` is not an uuid, add it as a label associated with
         uuid to the jstore */
      const char *id = src + t->start;
      int len = t->end - t->start;
      int uuidver = dlite_get_uuidn(uuid, id, len);
      if (uuidver < 0)
        goto fail;
      else if (uuidver > 0)
        jstore_set_labeln(js, uuid, id, len);

      if (jstore_addn(js, uuid, DLITE_UUID_LENGTH,
                      src + v->start, v->end - v->start)) goto fail;
      t += jsmn_count(v) + 2;
    }
  }
  retval = format;

 fail:
  if (tokens) free(tokens);
  if (uri) free(uri);
  return retval;
}

/*
  Read content of `filename` to json store `js`.

  Returns json format or -1 on error.
 */
DLiteJsonFormat dlite_jstore_loadf(JStore *js, const char *filename)
{
  char *buf = jstore_readfile(filename);
  int fmt;
  if (!buf) return err(dliteStorageLoadError, "cannot load JSON file \"%s\"",
                       filename);
  fmt = dlite_jstore_loads(js, buf, strlen(buf));
  free(buf);
  return fmt;
}

/*
  Add json representation of `inst` to json store `js`.

  Returns non-zero on error.
 */
int dlite_jstore_add(JStore *js, const DLiteInstance *inst, DLiteJsonFlag flags)
{
  char *buf;
  if (!(buf = dlite_json_aprint(inst, 2, flags | dliteJsonSingle)))
    return -1;
  return jstore_addstolen(js, inst->uuid, buf);
}

/*
  Removes instance with given id from json store `js`.

  Returns non-zero on error.
 */
int dlite_jstore_remove(JStore *js, const char *id)
{
  return jstore_remove(js, id);
}

/*
  Returns instance with given id from json store `js` or NULL on error.
*/
DLiteInstance *dlite_jstore_get(JStore *js, const char *id)
{
  char uuid[DLITE_UUID_LENGTH+1];
  const char *buf=NULL, *scanid=id;
  int uuidver = dlite_get_uuid(uuid, id);
  if (uuidver < 0 || uuidver == UUID_RANDOM)
    return errx(dliteKeyError, "cannot derive UUID from id: '%s'", id), NULL;
  if (!(buf = jstore_get(js, uuid)) &&
      !(buf = jstore_get(js, id)))
    return errx(dliteKeyError, "no such id in store: '%s'", id), NULL;

  /* If `id` is an UUID, check if `id` has been associated with a label */
  if ((uuidver == UUID_COPY || uuidver == UUID_EXTRACT) &&
      !(scanid = jstore_get_label(js, id))) scanid = id;

  return dlite_json_sscan(buf, scanid, NULL);
}

/*
  Initiate iterator `init` from json store `js`.
  If `metaid` is provided, the iterator will only iterate over instances
  of this metadata.

  Returns a new iterator or NULL on error.
 */
DLiteJStoreIter *dlite_jstore_iter_create(JStore *js, const char *metaid)
{
  DLiteJStoreIter *iter=NULL;
  if (!(iter = calloc(1, sizeof(DLiteJStoreIter))))
    FAILCODE(dliteMemoryError, "allocation failure");
  if (jstore_iter_init(js, &iter->jiter)) goto fail;
  if (metaid && dlite_get_uuid(iter->metauuid, metaid) < 0) goto fail;
 fail:
  return iter;
}

/*
  Free iterater.  Returns non-zero on error.
*/
int dlite_jstore_iter_free(DLiteJStoreIter *iter)
{
  jstore_iter_deinit(&iter->jiter);
  if (iter->tokens) free(iter->tokens);
  free(iter);
  return 0;
}

/*
  Return the id of the next instance in the json store or NULL if the
  iterator is exausted.
 */
const char *dlite_jstore_iter_next(DLiteJStoreIter *iter)
{
  const char *iid;
  JStore *js = iter->jiter.js;
  jsmn_parser parser;
  while ((iid = jstore_iter_next(&iter->jiter))) {
    if (iter->metauuid[0]) {
      char metauuid[DLITE_UUID_LENGTH+1];
      const char *val = jstore_get(js, iid);
      int r;

      jsmn_init(&parser);
      if ((r = jsmn_parse_alloc(&parser, val, strlen(val),
                                &iter->tokens, &iter->ntokens)) < 0) {
        if (r == JSMN_ERROR_INVAL)
          err(dliteParseError, "invalid json input: \"%s\"", val);
        else
          err(dliteParseError, "json parse error: \"%s\"", jsmn_strerror(r));
        continue;
      }
      if (get_meta_uuid(metauuid, val, iter->tokens)) {
        err(dliteMissingMetadataError,
            "json input has no meta uri: \"%s\"", val);
        continue;
      }
      if (strcmp(metauuid, iter->metauuid)) continue;
    }
    return iid;
  }
  return NULL;
}
