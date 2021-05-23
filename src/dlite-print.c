#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

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
#define PRINT4(fmt, a1, a2, a3, a4)                                 \
  do {                                                              \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1, a2, a3, a4);  \
    if (m < 0) goto fail;                                           \
    n += m;                                                         \
  } while (0)



/*
  Serialise instance `inst` to `dest`, formatted as JSON.

  No more than `size` bytes are written to `dest` (incl. the
  terminating NUL).

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `size`, the number of bytes that would
  have been written if `size` was large enough is returned.  On error, a
  negative value is returned.
*/
int dlite_sprint(char *dest, size_t size, DLiteInstance *inst,
                 int indent, DLitePrintFlag flags)
{
  DLiteTypeFlag f = (DLiteTypeFlag)flags;
  int n=0, ok=0, m, j;
  size_t i;
  char *in = malloc(indent + 1);
  memset(in, ' ', indent);
  in[indent] = '\0';

  PRINT1("%s{\n", in);
  if (flags & dlitePrintUUID)
    PRINT2("%s  \"uuid\": \"%s\",\n", in, inst->uuid);
  if (inst->uri)
    PRINT2("%s  \"uri\": \"%s\",\n", in, inst->uri);
  PRINT2("%s  \"meta\": \"%s\",\n", in, inst->meta->uri);

  if ((flags & dlitePrintMetaAsData) || dlite_instance_is_data(inst)) {
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
  Like dlite_sprint(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*size`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
 */
int dlite_asprint(char **dest, size_t *size, size_t pos, DLiteInstance *inst,
                  int indent, DLitePrintFlag flags)
{
  int m;
  void *q;
  size_t newsize;
  if (!dest && !*dest) *size = 0;
  m = dlite_sprint(*dest + pos, PDIFF(*size, pos), inst, indent, flags);
  if (m < 0) return m;
  if (m < (int)PDIFF(*size, pos)) return m;

  /* Reallocate buffer to required size. */
  newsize = m + pos + 1;
  if (!(q = realloc(*dest, newsize))) return -1;
  *dest = q;
  *size = newsize;
  m = dlite_sprint(*dest + pos, PDIFF(*size, pos), inst, indent, flags);
  assert(0 <= m && m < (int)*size);
  return m;
}


/*
  Like dlite_sprint(), but returns allocated buffer with serialised instance.
 */
char *dlite_aprint(DLiteInstance *inst, int indent, DLitePrintFlag flags)
{
  char *dest=NULL;
  size_t size=0;
  if ((dlite_asprint(&dest, &size, 0, inst, indent, flags)) < 0) return NULL;
  return dest;
}

/*
  Like dlite_sprint(), but prints to stream `fp`.

  Returns number or bytes printed or a negative number on error.
 */
int dlite_fprint(FILE *fp, DLiteInstance *inst, int indent,
                 DLitePrintFlag flags)
{
  int m;
  char *buf=NULL;
  size_t size=0;
  if ((m = dlite_asprint(&buf, &size, 0, inst, indent, flags)) >= 0) {
    fprintf(fp, "%s\n", buf);
    free(buf);
  }
  return m;
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
  char *buf=NULL;
  size_t i, size=0, *dims=NULL;
  DLiteInstance *inst=NULL;
  const DLiteMeta *meta;
  jsmntype_t dimtype = 0;
  char *name=NULL, *version=NULL, *namespace=NULL;

  assert(obj->type == JSMN_OBJECT);

  printf("\n--- parse %s\n", id);

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
        !(meta = (DLiteMeta *)dlite_sscan(src, buf)))
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

  /* Create instance */
  if (!(inst = dlite_instance_create(meta, dims, id))) goto fail;


  dlite_instance_debug(inst);

  /* FIXME - should have been called by dlite_instance_create() */
  if (dlite_instance_is_meta(inst)) {
    DLiteMeta *m = (DLiteMeta *)inst;
    size_t npropdims=0;
    for (i=0; i < meta->_nproperties; i++)
      npropdims += meta->_properties[i].ndims;
    m->_npropdims = npropdims;
    printf("*** npropdims=%zu\n", npropdims);
    dlite_meta_init((DLiteMeta *)inst);
    size_t *dimensions = DLITE_DIMS(inst);
    memcpy(dimensions, dims, meta->_ndimensions*sizeof(size_t));
  }

  dlite_instance_debug(inst);


  if (dlite_meta_is_metameta(meta)) {
    DLiteMeta *m = (DLiteMeta *)inst;
    size_t *dimensions = DLITE_DIMS(inst);
    memcpy(dimensions, dims, meta->_ndimensions*sizeof(size_t));
    dlite_meta_init(m);
    printf("*** npropdims=%zu\n", m->_npropdims);
    m->_npropdims = 1;
    dlite_meta_init(m);
    printf("*** npropdims=%zu\n", m->_npropdims);
  }


  /* Parse properties */
  if (meta->_nproperties > 0) {
    jsmntok_t *base;
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
    } else if (dlite_split_meta_uri(id, &name, &version, &namespace))
      FAIL1("cannot infer name, version and namespace from id: %s", id);

    /* -- read properties */
    for (i=0; i < meta->_nproperties; i++) {
      void *ptr;
      DLiteProperty *p = meta->_properties + i;
      size_t *pdims = DLITE_PROP_DIMS(inst, i);

      printf("  * prop: %s\n", p->name);

      if (!(ptr = dlite_instance_get_property_by_index(inst, i))) goto fail;

      if ((t = jsmn_item(src, base, p->name))) {
        strnput(&buf, &size, 0, src+t->start, t->end-t->start);


        printf("  - scanning %s, size=%d, type=%d.%d\n",
               p->name, t->size, p->type, (int)p->size);

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
      printf("-\n");
    }

        ///* spacial cases for metadata... */
        //if (dimtype == JSMN_ARRAY &&
        //    (strcmp(p->name, "dimensions") == 0 ||
        //     strcmp(p->name, "properties") == 0 ||
        //     strcmp(p->name, "relations") == 0)) {
        //  int j, len=t->size;
        //  assert(t->type == dimtype);
        //  assert(p->ndims == 1);
        //  //assert(t->size == (int)pdims[0]);
        //
        //  printf("  - scanning %s, len=%d, type=%d.%d: '%.*s'\n",
        //         p->name, len, p->type, (int)p->size, t->end-t->start, src+t->start);
        //
        //  for (j=0; j<len; j++) {
        //    t++;
        //    strnput(&buf, &size, 0, src+t->start, t->end-t->start);
        //    printf("  = value%d = '%s'\n", j, buf);
        //    if (dlite_property_scan(buf, ptr, p, pdims, 0) < 0)
        //      goto fail;
        //    t += jsmn_count(t);
        //  }
        //} else {
        //  if (dlite_property_scan(src+t->start, ptr, p, pdims, 0) < 0)
        //    goto fail;
        //}


      //if ((t = jsmn_item(src, base, p->name))) {
      //
      //  printf("  * val: '%.*s'\n", t->end-t->start, src+t->start);
      //
      //  if (dlite_property_scan(src+t->start, ptr, p, pdims, 0) < 0) goto fail;
      //} else if (strcmp(p->name, "name") == 0) {
      //  if (dlite_property_scan(name, ptr, p, pdims, 0) < 0) goto fail;
      //} else if (strcmp(p->name, "version") == 0) {
      //  if (dlite_property_scan(version, ptr, p, pdims, 0) < 0) goto fail;
      //} else if (strcmp(p->name, "namespace") == 0) {
      //  if (dlite_property_scan(namespace, ptr, p, pdims, 0) < 0) goto fail;
      //} else
      //  FAIL2("missing property \"%s\" in %s", p->name, id);

      //printf("  * key: '%.*s'\n", t->end-t->start, src+t->start);
      //jsmntok_t *v = t + 1;
      //printf("    val: '%.*s'\n", v->end-v->start, src+v->start);
      //
      //if (t->type != JSMN_STRING)
      //  FAIL1("property keys should be strings: %s", id);
      //
      //
      //if (dlite_property_scan(src+v->start, ptr, p, pdims, 0) < 0) goto fail;
      //t += jsmn_count(v) + 2;




    //if (item->type == JSMN_OBJECT) {
    //  for (i=0, t=item+1; i < meta->_nproperties; i++) {
    //    void *ptr;
    //    DLiteProperty *p = meta->_properties + i;
    //    size_t *pdims = DLITE_PROP_DIMS(inst, i);
    //    jsmntok_t *v = t+1;
    //    if (t->type != JSMN_STRING)
    //      FAIL1("dimension keys should be strings: %s", id);
    //    printf("  - %.*s: '%.*s'\n",
    //           t->end-t->start, src+t->start,
    //           v->end-v->start, src+v->start);
    //    if (!(ptr = dlite_instance_get_property_by_index(inst, i))) goto fail;
    //    if (dlite_property_scan(src+v->start, ptr, p, pdims, 0) < 0) goto fail;
    //    t += jsmn_count(v) + 2;
    //  }
    //} else if (item->type == JSMN_ARRAY) {
    //  assert(dlite_instance_is_meta(inst));
    //  for (i=0, t=item+1; (int)i < item->size; i++) {
    //    void *ptr;
    //    DLiteProperty *p = meta->_properties + i;
    //    size_t *pdims = DLITE_PROP_DIMS(inst, i);
    //    printf("  - prop%d='%.*s'\n", (int)i, t->end-t->start, src+t->start);
    //    if (!(ptr = dlite_instance_get_property_by_index(inst, i))) goto fail;
    //    if (dlite_property_scan(src+t->start, ptr, p, pdims, 0) < 0) goto fail;
    //    t += jsmn_count(t) + 1;
    //  }
    //} else {
    //  FAIL1("\"properties\" must be object or array: %s", id);
    //}

  }

  printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
  dlite_fprint(stdout, inst, 0, 0);

  ok = 1;
 fail:
  if (name) free(name);
  if (version) free(version);
  if (namespace) free(namespace);
  if (dims) free(dims);
  if (buf) free(buf);
  if (!ok && inst) {
    dlite_instance_decref(inst);
    inst = NULL;
  }
  return inst;
}


/*
  Returns a new instance scanned from `src`.

  The `flags` provides some format options.  If zero (default) bools
  and strings are expected to be quoted.
 */
DLiteInstance *dlite_sscan(const char *src, const char *id)
{
  int i, r;
  DLiteInstance *inst=NULL;
  unsigned int ntokens=0;
  jsmntok_t *tokens=NULL, *root;
  jsmn_parser parser;
  UNUSED(id);
  jsmn_init(&parser);
  r = jsmn_parse_alloc(&parser, src, strlen(src), &tokens, &ntokens);
  if (r < 0) FAIL1("error parsing json: %s", jsmn_strerror(r));
  root = tokens;
  if (root->type != JSMN_OBJECT) FAIL("json root should be an object");

  if (!id) {
    inst = parse_instance(src, root, id);
  } else {
    int n=1;
    char uuid[DLITE_UUID_LENGTH+1];
    if (dlite_get_uuid(uuid, id) < 0) goto fail;
    for (i=0; i < root->size; i++) {
      char uuid2[DLITE_UUID_LENGTH+1], buf[128];
      jsmntok_t *key = root + n;
      jsmntok_t *val = root + n+1;
      int len = key->end - key->start;
      if (key->type != JSMN_STRING) FAIL("expect json keys to be strings");
      if (len >= (int)sizeof(buf))
        FAIL3("key exceeded maximum key length (%d): %.*s",
              (int)sizeof(buf), len, src+key->start);
      strncpy(buf, src+key->start, len);
      buf[len] = '\0';
      if (dlite_get_uuid(uuid2, buf) < 0) goto fail;
      if (strcmp(uuid2, uuid) == 0) {
        inst = parse_instance(src, val, id);
        break;
      }
      n += jsmn_count(val) + 2;
    }
  }
 fail:
  free(tokens);

  printf("--------------> %s\n", id);
  return inst;
}


DLiteInstance *dlite_fscan(FILE *fp, const char *id)
{
  DLiteInstance *inst;
  char *buf;
  if (!(buf = fu_readfile(fp))) return NULL;
  inst = dlite_sscan(buf, id);
  free(buf);
  return inst;
}
