/* jstore.c -- simple JSON storage
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#include "err.h"
#include "compat.h"
#include "jstore.h"


#define FAIL(msg) do {                          \
    err(1, msg); goto fail; } while (0)


/* JSON store */
struct _JStore {
  map_str_t store;       // maps keys to json content
  map_str_t labels;      // maps keys to associated label
};


/*
 * Utility functions
 * -----------------
 */


/* Read stream into an allocated buffer.
   Returns a pointer to the buffer or NULL on error. */
char *jstore_readfp(FILE *fp)
{
  char *q, *buf=NULL;
  size_t n, bytes_left, bytes_read=0, size=256;
  do {
    if (ferror(fp)) FAIL("stream error");
    size *= 2;
    if (!(q = realloc(buf, size))) FAIL("reallocation failure");
    buf = q;
    bytes_left = size - bytes_read;
    n = fread(buf+bytes_read, 1, bytes_left, fp);
    if (ferror(fp)) FAIL("cannot read from stream. Is it a regular file "
                         "with read permissions?");
    bytes_read += n;
  } while (n == bytes_left && !feof(fp));
  assert(feof(fp));  // stream should be exausted
  if (!(q = realloc(buf, bytes_read+1))) FAIL("reallocation failure");
  buf = q;
  buf[bytes_read] = '\0';
  return buf;
 fail:
  if (buf) free(buf);
  return NULL;
}

/* Read file into an allocated buffer.
   Returns a pointer to the buffer or NULL on error. */
char *jstore_readfile(const char *filename)
{
  char *buf;
  FILE *fp = fopen(filename, "r");
  if (!fp) return err(1, "cannot open file: \"%s\"", filename), NULL;
  buf = jstore_readfp(fp);
  fclose(fp);
  if (!buf) err(1, "error reading from file \"%s\"", filename);
  return buf;
}

/* Read file into an allocated buffer and parse it with JSMN.

   The `tokens_ptr` and `num_tokens_ptr` arguments are passed on to
   jsmn_parse_alloc().

   Returns a pointer to the buffer or NULL on error. */
char *jstore_readfile_to_jsmn(const char *filename, jsmntok_t **tokens_ptr,
                              unsigned int *num_tokens_ptr)
{
  char *buf = jstore_readfile(filename);
  jsmn_parser parser;
  int r;
  if (!buf) return NULL;
  jsmn_init(&parser);
  r = jsmn_parse_alloc(&parser, buf, strlen(buf), tokens_ptr, num_tokens_ptr);
  if (r < 0) {
    free(buf);
    return errx(1, "error parsing json file \"%s\": %s",
                filename, jsmn_strerror(r)), NULL;
  }
  return buf;
}


/*
 * JStore API
 * ----------
 */

/* Create a new JSON store and return it.  Returns NULL on error. */
JStore *jstore_open(void)
{
  JStore *js = calloc(1, sizeof(JStore));
  if (!js) return err(1, "allocation failure"), NULL;
  map_init(&js->store);
  map_init(&js->labels);
  return js;
}

/* Close JSON store.  Returns non-zero on error. */
int jstore_close(JStore *js)
{
  const char *key;

  map_iter_t miter = map_iter(&js->store);
  while ((key = map_next(&js->store, &miter))) {
    char **val = map_get(&js->store, key);
    assert(val);
    free(*val);
  }
  map_deinit(&js->store);

  map_iter_t liter = map_iter(&js->labels);
  while ((key = map_next(&js->labels, &liter))) {
    char **val = map_get(&js->labels, key);
    assert(val);
    free(*val);
  }
  map_deinit(&js->labels);

  free(js);
  return 0;
}

/* Add JSON value to store with given key.
   If key already exists, it is replaced.
   Returns non-zero on error. */
int jstore_add(JStore *js, const char *key, const char *value)
{
  return jstore_addn(js, key, 0, value, 0);
}

/* Add JSON value to store with given key.
   The lengths of the key and value are provided by `klen` and `vlen`,
   respectively.  If the strings are NUL-terminated, the corresponding length
   may be set to zero.
   If key already exists, it is replaced.
   Returns non-zero on error. */
int jstore_addn(JStore *js, const char *key, size_t klen,
                const char *value, size_t vlen)
{
  char *k=(char *)key, *v=NULL;
  int stat;
  if (!vlen) vlen = strlen(value);
  if (klen && !(k = strndup(key, klen))) FAIL("allocation failure");
  if (!(v = strndup(value, vlen))) FAIL("allocation failure");
  stat = jstore_addstolen(js, k, v);
  if (klen) free(k);
  return stat;
 fail:
  if (klen) free(k);
  if (v) free(v);
  return 1;
}

/* Add JSON value to store with given key.
   The store "steels" the ownership of the memory pointed to by `value`.
   If key already exists, it is replaced.
   Returns non-zero on error. */
int jstore_addstolen(JStore *js, const char *key, const char *value)
{
  char **v;
  if ((v = map_get(&js->store, key))) free(*v);  // free existing value
  if (map_set(&js->store, key, (char *)value))
    return err(1, "error adding key \"%s\" to JSON store", key);
  return 0;
}

/* Returns JSON value for given key or NULL if the key isn't in the store.
   This function can also be used to check if a key exists in the store. */
const char *jstore_get(JStore *js, const char *key)
{
  char **p = map_get(&js->store, key);
  return (p) ? *p : NULL;
}

/* Removes item corresponding to given key from JSON store.
   Returns zero on success and 1 if `key` doesn't exists. */
int jstore_remove(JStore *js, const char *key)
{
  char **v;
  if ((v = map_get(&js->store, key))) {
    free(*v);
    map_remove(&js->store, key);
    return 0;
  }
  return 1;
}

/* Update JSON store with values from `other`. Return non-zero on error. */
int jstore_update(JStore *js, JStore *other)
{
  JStoreIter iter;
  const char *key;
  jstore_iter_init(other, &iter);
  while ((key = jstore_iter_next(&iter))) {
    const char *val = jstore_get(other, key);
    assert(val);
    if (jstore_add(js, key, val)) return 1;
  }
  return 0;
}

/* Update JSON store with values from JSMN token `tok`. The JSMN token must be
   an object. Return non-zero on error. */
int jstore_update_from_jsmn(JStore *js, const char *src, jsmntok_t *tok)
{
  int i;
  jsmntok_t *tk = tok+1;
  if (tok->type != JSMN_OBJECT) return err(1, "jsmn token must be an object");
  for (i=0; i < tok->size; i++) {
    jsmntok_t *tv = tk+1;
    if (jstore_addn(js,
                    src + tk->start, tk->end - tk->start,
                    src + tv->start, tv->end - tv->start)) return 1;
    tk += jsmn_count(tv) + 2;
  }
  return 0;
}

/* Update JSON store with values from string `buf`.
   Return non-zero on error. */
int jstore_update_from_string(JStore *js, const char *buf, int len)
{
  jsmn_parser parser;
  jsmntok_t *tokens=NULL;
  unsigned int ntokens=0;
  int r, stat;
  jsmn_init(&parser);
  r = jsmn_parse_alloc(&parser, buf, len, &tokens, &ntokens);
  if (r < 0) {
    return err(1, "error parsing JSON buffer \"%.70s\": %s",
               buf, jsmn_strerror(r));
  }
  stat = jstore_update_from_jsmn(js, buf, tokens);
  free(tokens);
  return stat;
}

/* Update JSON store with values from file `filename`.
   Return non-zero on error. */
int jstore_update_from_file(JStore *js, const char *filename)
{
  int stat;
  char *buf;
  if (!(buf = jstore_readfile(filename)))
    return err(1, "error reading JSON file \"%s\"", filename);
  stat = jstore_update_from_string(js, buf, strlen(buf));
  free(buf);
  return stat;
}

/* Update `filename` from JSON store.
   The file is first read and then rewritten while the store is unchanged.
   Return non-zero on error. */
int jstore_update_file(JStore *js, const char *filename)
{
  int retval=1;
  JStore *js2=NULL;
  if (!(js2 = jstore_open())) goto fail;
  if (jstore_update_from_file(js2, filename)) goto fail;
  if (jstore_update(js2, js)) goto fail;
  if (jstore_to_file(js2, filename)) goto fail;
  retval = 0;
 fail:
  if (js2) jstore_close(js2);
  return retval;
}

/* Returns a malloc()'ed JSON string with the content of the store or
   NULL on error. */
char *jstore_to_string(JStore *js)
{
  map_iter_t iter = map_iter(&js->store);
  char *buf=NULL;
  size_t size=0;
  int m, n=0, count=0;
  const char *key;
  if ((m = asnpprintf(&buf, &size, n, "{")) < 0) goto fail;
  n += m;
  while ((key = map_next(&js->store, &iter))) {
    char **q, *sep = (count++ > 0) ? "," : "";
    if (!(q = map_get(&js->store, key))) goto fail;
    if ((m = asnpprintf(&buf, &size, n, "%s\n  \"%s\": %s",
                        sep, key, *q)) < 0) goto fail;
    n += m;
  }
  if ((m = asnpprintf(&buf, &size, n, "\n}\n")) < 0) goto fail;
  n += m;
  return buf;
 fail:
  return err(1, "error creating json string"), NULL;
}

/* Writes JSON store to file.  If `filename` exists, it is overwritten.
   Returns non-zero on error. */
int jstore_to_file(JStore *js, const char *filename)
{
  char *buf = jstore_to_string(js);
  FILE *fp;
  size_t n;
  if (!buf) return 1;
  if (!(fp = fopen(filename, "w"))) {
    free(buf);
    return err(1, "cannot write JSON store to file \"%s\"", filename);
  }
  n = fwrite(buf, strlen(buf), 1, fp);
  fclose(fp);
  free(buf);
  return (n == 1) ? 0 : 1;
}

/* Return number of elements in the store. */
int jstore_count(JStore *js)
{
  int n=0;
  map_iter_t iter = map_iter(&js->store);
  while (map_next(&js->store, &iter)) n++;
  return n;
}

/* If there is one item in the store, return its key.  Otherwise return NULL. */
const char *jstore_get_single_key(JStore *js)
{
  map_iter_t iter = map_iter(&js->store);
  const char *key = map_next(&js->store, &iter);
  if (key && !map_next(&js->store, &iter)) return key;
  return NULL;
}

/* Initialise iterator.  Return non-zero on error. */
int jstore_iter_init(JStore *js, JStoreIter *iter)
{
  iter->js = js;
  iter->miter = map_iter(&js->store);
  return 0;
}

/* Returns the key to the next item in or NULL if there are no more items
   in the store.  Use jstore_get() to get the corresponding value. */
const char *jstore_iter_next(JStoreIter *iter)
{
  return map_next(&iter->js->store, &iter->miter);
}

/* Deinitialise the iterator.  Returns non-zero on error.
   In the current implementation, this function does nothing. */
int jstore_iter_deinit(JStoreIter *iter)
{
  (void)(iter);  // unused
  return 0;
}


/* Associate `label` with `key`.  If `len` is non-negative, it is the
   length of `label`; otherwise `label` is assumed to be NUL-terminated.
   If `key` already has a label, the old label is replaced.
   Returns non-zero on error. */
int jstore_set_labeln(JStore *js, const char *key, const char *label, int len)
{
  char **p, *s;
  int stat;
  if ((p = map_get(&js->labels, key))) free(*p);

  if (len > 0)
    s = strndup(label, len);
  else if (len == 0)
    s = strdup("");
  else
    s = strdup(label);
  if (!s) return err(1, "allocation failure");

  if ((stat = map_set(&js->labels, key, s))) {
    errx(2, "error associating label '%s' to key '%s'", s, key);
    free(s);
  }
  return stat;
}

/* Associate `label` with `key`.
   If `key` already has a label, the old label is replaced.
   Returns non-zero on error. */
int jstore_set_label(JStore *js, const char *key, const char *label)
{
  return jstore_set_labeln(js, key, label, -1);
}

/* Returns a pointer to label associated with `key` or NULL if `key` has no
   associated label. */
const char *jstore_get_label(JStore *js, const char *key)
{
  char **p = map_get(&js->labels, key);
  if (!p) return NULL;
  return (const char *)*p;
}
