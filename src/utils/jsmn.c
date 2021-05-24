/* jsmn.h -- small and fast JSON parser, see https://github.com/zserge/jsmn */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"

/* Include the jsmn.h header without defining JSMN_HEADER defined.
   This defines all functions in jsmn. */
#define JSNM_STATIC
#include "jsmn.h"


/*
  Additional functions not provided with the minimalistic jsmn api
  ================================================================

  We allow dependencies on the standard library in this file.
 */


/*
  Like jsmn_parse(), but realloc's the buffer pointed to by `tokens_ptr`
  if it is too small.  `num_tokens_ptr` should point to the number of
  allocated tokens.

  Returns JSMN_ERROR_NOMEM on allocation error.
 */
int jsmn_parse_alloc(jsmn_parser *parser, const char *js, const size_t len,
                     jsmntok_t **tokens_ptr, unsigned int *num_tokens_ptr)
{
  int n, n_save;
  unsigned int saved_pos;
  jsmntok_t *t=NULL;
  (void) n_save;  // avoid unused parameter error when assert is turned off
  assert(tokens_ptr);
  assert(num_tokens_ptr);
  if (!*tokens_ptr) *num_tokens_ptr = 0;
  if (!*num_tokens_ptr) *tokens_ptr = NULL;

  saved_pos = parser->pos;
  if ((n = jsmn_parse(parser, js, len, NULL, 0)) < 0) goto fail;
  if (!(t = realloc(*tokens_ptr, n*sizeof(jsmntok_t)))) return JSMN_ERROR_NOMEM;
  n_save = n;
  parser->pos = saved_pos;
  if ((n = jsmn_parse(parser, js, len, t, n)) < 0) goto fail;
  assert(n == n_save);
  *tokens_ptr = t;
  *num_tokens_ptr = n;
  return n;
 fail:
  if (t) free(t);
  switch (n) {
  case JSMN_ERROR_NOMEM: abort();  // this should never happen
  case JSMN_ERROR_INVAL: return JSMN_ERROR_INVAL;
  case JSMN_ERROR_PART: return JSMN_ERROR_INVAL;
  }
  abort();  // should never be reached
}


/*
  Returns number of sub-tokens contained in `t` or -1 on error.
*/
int jsmn_count(const jsmntok_t *t)
{
  int n=0, i;
  switch (t->type) {
  case JSMN_UNDEFINED:
  case JSMN_STRING:
  case JSMN_PRIMITIVE:
    return 0;
  case JSMN_OBJECT:
    for (i=0; i < t->size; i++) {
      n++;
      assert(t[n].type == JSMN_STRING);  // keys must be strings
      n++;
      n += jsmn_count(t+n);
    }
    return n;
  case JSMN_ARRAY:
    for (i=0; i < t->size; i++) {
      n++;
      n += jsmn_count(t+n);
    }
    return n;
  }
  abort();
}


/*
  Returns a pointer to the value of item `key` of JSMN object token `t`.

  `js` is the JSON source.

  Returns NULL on error.
*/
jsmntok_t *jsmn_item(const char *js, jsmntok_t *t, const char *key)
{
  int i, n, nitems;
  int len, keylen=strlen(key);
  if (t->type != JSMN_OBJECT) return errx(1, "expected JSMN OBJECT"), NULL;
  nitems = t->size;
  for (i=0; i<nitems; i++) {
    t++;
    assert(t->type == JSMN_STRING);
    len = t->end - t->start;
    if (len == keylen && strncmp(key, js + t->start, len) == 0) return t+1;
    t++;
    if ((n = jsmn_count(t)) < 0) return NULL;
    t += n;
  }
  return NULL;  // no such key
}


/*
  Returns a pointer to the element `i` of JSMN array token `t`.

  `js` is the JSON source.

  Returns NULL on error.
*/
jsmntok_t *jsmn_element(const char *js, jsmntok_t *t, int i)
{
  int j, n;
  (void)js;  // unused
  if (t->type != JSMN_ARRAY)
    return errx(1, "expected JSMN ARRAY"), NULL;
  if (i < 0 || i >= t->size)
    return errx(1, "element i=%d is out of range [0:%d]", i, t->size-1), NULL;
  for (j=0; j<i; j++) {
    t++;
    if ((n = jsmn_count(t)) < 0) return NULL;
    t += n;
  }
  return t+1;
}


/*
  Returns error message corresponding to return value from jsmn_parse().
*/
JSMN_API const char *jsmn_strerror(int r)
{
  if (r >= 0) return "success";
  switch (r) {
  case JSMN_ERROR_INVAL:
    return "bad token, JSON string is corrupted";
  case JSMN_ERROR_NOMEM:
    return "not enough tokens, JSON string is too large";
  case JSMN_ERROR_PART:
    return "JSON string is too short, expecting more JSON data";
  default:
    return "unknown error";
  }
}
