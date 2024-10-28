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
#include "jsmnx.h"

/*
  Like jsmn_parse(), but realloc's the buffer pointed to by `tokens_ptr`
  if it is too small.  `num_tokens_ptr` should point to the number of
  allocated tokens.

  Returns number of tokens used by the parser or one of the following error
  codes on error:
    - JSMN_ERROR_NOMEM on allocation error.
    - JSMN_INVAL on invalid character inside json string.
 */
int jsmn_parse_alloc(jsmn_parser *parser, const char *js, const size_t len,
                     jsmntok_t **tokens_ptr, unsigned int *num_tokens_ptr)
{
  int n_tokens;
  jsmntok_t *tokens=NULL;
  assert(tokens_ptr);
  assert(num_tokens_ptr);
  if (!*num_tokens_ptr) *tokens_ptr = NULL;
  if (!*tokens_ptr) *num_tokens_ptr = 0;

  if (!*tokens_ptr) {
    if ((n_tokens = jsmn_required_tokens(js, len)) < 0) return n_tokens;

    /* FIXME: there seems to be an issue with the dlite_json_check() that
       looks post the last allocated token. Allocating `n+1` tokens is a
       workaround to avoid memory issues. */
    if (!(tokens = calloc(n_tokens+1, sizeof(jsmntok_t))))
      return JSMN_ERROR_NOMEM;
  } else {
    jsmn_parser saved_parser;
    memcpy(&saved_parser, parser, sizeof(saved_parser));
    n_tokens = jsmn_parse(parser, js, len, *tokens_ptr, *num_tokens_ptr);
    if (n_tokens >= 0 || n_tokens != JSMN_ERROR_NOMEM) return n_tokens;

    // Try to handle JSMN_ERROR_NOMEM by reallocating
    if ((n_tokens = jsmn_required_tokens(js, len)) < 0) return n_tokens;
    if (!(tokens = realloc(*tokens_ptr, (n_tokens+1)*sizeof(jsmntok_t))))
      return JSMN_ERROR_NOMEM;

    // Resetting parser - is this really needed?
    memcpy(parser, &saved_parser, sizeof(saved_parser));
  }
  *tokens_ptr = tokens;
  *num_tokens_ptr = n_tokens;

  n_tokens = jsmn_parse(parser, js, len, tokens, n_tokens);
  assert(n_tokens != JSMN_ERROR_NOMEM);
  return n_tokens;
}


/*
  Returns number of tokens required to parse JSON string `js` of length `len`.
  On error, JSMN_ERROR_INVAL or JSMN_ERROR_PART is retuned.
 */
int jsmn_required_tokens(const char *js, size_t len)
{
  int n_tokens;
  jsmn_parser parser;
  jsmn_init(&parser);
  n_tokens = jsmn_parse(&parser, js, len, NULL, 0);
  assert(n_tokens != JSMN_ERROR_NOMEM);
  return n_tokens;
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
