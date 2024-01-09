/* jsmnx.c -- extended version of the simple JSMN JSON parser
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"

/* Include the jsmn.h header without defining JSMN_HEADER defined.
   This defines all functions in jsmn. */
#define JSNM_STATIC
#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#include "jsmn.h"


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
  if (!*num_tokens_ptr) *tokens_ptr = NULL;
  if (!*tokens_ptr) *num_tokens_ptr = 0;

  saved_pos = parser->pos;

  if (!*tokens_ptr) {
    if ((n = jsmn_parse(parser, js, len, NULL, 0)) < 0) goto fail;
    /* FIXME: there seems to be an issue with the dlite_json_check() that
       looks post the last allocated token. Allocating `n+1` tokens is a
       workaround to avoid memory issues. */
    if (!(t = calloc(n+1, sizeof(jsmntok_t)))) return JSMN_ERROR_NOMEM;
  } else {
    n = jsmn_parse(parser, js, len, *tokens_ptr, *num_tokens_ptr);
    if (n >= 0) return n;
    if (n != JSMN_ERROR_NOMEM) goto fail;
    if (!(t = realloc(*tokens_ptr, n*sizeof(jsmntok_t))))
      return JSMN_ERROR_NOMEM;
  }
  *tokens_ptr = t;
  *num_tokens_ptr = n;
  n_save = n;

  /* TODO: Instead of resetting the parser, we should continue after
     reallocation */
  parser->pos = saved_pos;
  if ((n = jsmn_parse(parser, js, len, t, n)) < 0) goto fail;
  assert(n == n_save);
  return n;
 fail:
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
const jsmntok_t *jsmn_item(const char *js, const jsmntok_t *t, const char *key)
{
  int i, n, nitems;
  int len, keylen=strlen(key);
  if (t->type != JSMN_OBJECT)
    return errx(1, "expected JSON object in string starting with:\n%.200s\n",
                js + t->start), NULL;
  nitems = t->size;
  for (i=0; i<nitems; i++) {
    t++;
    len = t->end - t->start;
    if (t->type != JSMN_STRING)
      return errx(1, "invalid JSON, object key must be a string, got '%.*s'",
                  len, js + t->start), NULL;
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
const jsmntok_t *jsmn_element(const char *js, const jsmntok_t *t, int i)
{
  int j, n;
  int len = t->end - t->start;
  if (t->type != JSMN_ARRAY)
    return errx(1, "expected JSON array, got '%.*s", len, js + t->start), NULL;
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
const char *jsmn_strerror(int r)
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
