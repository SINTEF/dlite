/* jsmnx.h -- extended version of the simple JSMN JSON parser
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */

/**
  @file
  @brief Extended version of the simple JSMN JSON parser

  This header provides a few useful additional functions in addition to
  those provided by jsmn.h.  When using jsmnx, you should import this file
  instead of jsmn.h in your project.

  We allow dependencies on the standard library in this file.

  For completeness, this file also include prototypes for jsmn_init() and
  jsmn_parse(), even though they are also declared in jsmn.h.

  @see https://github.com/zserge/jsmn
*/
#ifndef JSMNX_H
#define JSMNX_H

#include <stddef.h>

#define JSMN_HEADER
#define JSMN_STRICT
#define JSMN_PARENT_LINKS
#include "jsmn.h"

/** Chunck size when reallocating new chunks */
#ifndef JSMN_CHUNK_SIZE
#define JSMN_CHUNK_SIZE 4096
#endif


/**
 * Initializes a JSON parser.
 */
void jsmn_init(jsmn_parser *parser);


/**
 * Run JSON parser.
 *
 * It parses a JSON data string into and array of tokens, each
 * describing a single JSON object.
 *
 * Arguments
 * ---------
 *  - parser: pointer to initialized parser
 *  - js: the JSON input string
 *  - len: length of `js`
 *  - tokens: pointer to a preallocated buffer of JSMN tokens.  May be
 *    NULL in order to return the number of tokens in the input.
 *  - num_tokens: the size of `tokens`
 *
 * Returns
 * -------
 * On success, it returns the number of tokens actually used by the parser.
 * On error, one of the following (negative) codes is returned:
 *
 *  - JSMN_ERROR_INVAL: bad token, JSON string is corrupted
 *  - JSMN_ERROR_NOMEM: not enough tokens, JSON string is too large
 *  - JSMN_ERROR_PART:  JSON string is too short, expecting more JSON data
 */
int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
               jsmntok_t *tokens, const unsigned int num_tokens);


/**
 * Like jsmn_parse(), but realloc's the buffer pointed to by `tokens_ptr`
 * if it is too small.  `num_tokens_ptr` should point to the number of
 * allocated tokens or be zero if `tokens_ptr` is not pre-allocated.
 *
 * Returns JSMN_ERROR_NOMEM on allocation error.
 */
int jsmn_parse_alloc(jsmn_parser *parser, const char *js,
                     const size_t len, jsmntok_t **tokens_ptr,
                     unsigned int *num_tokens_ptr);


/**
 * Returns number of sub-tokens contained in `t` or -1 on error.
 */
int jsmn_count(const jsmntok_t *t);


/**
 * Returns a pointer to the value of item `key` of JSMN object token `t`.
 *
 * `js` is the JSON source.
 *
 * Returns NULL on error.
 */
const jsmntok_t *jsmn_item(const char *js, const jsmntok_t *t, const char *key);


/**
 * Returns a pointer to the element `i` of JSMN array token `t`.
 *
 * `js` is the JSON source.
 *
 * Returns NULL on error.
 */
const jsmntok_t *jsmn_element(const char *js, const jsmntok_t *t, int i);


/**
 * Returns error message corresponding to return value from jsmn_parse().
 */
const char *jsmn_strerror(int r);


#endif /* JSMNX_H */
