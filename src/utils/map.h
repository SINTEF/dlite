/*
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef MAP_H
#define MAP_H

/**
  @file
  @brief A type-safe hash map implementation for C

  See https://github.com/rxi/map for the official documentation.

  ### Prototypes for provided macros

      typedef map_t(T) MAP_T;
  Creates a map struct for containing values of type T.

      void map_init(MAP_T *m);
  Initialises the map, this must be called before the map can be used.

      void map_deinit(MAP_T *m);
  Deinitialises the map, freeing the memory the map allocated
  during use; this should be called when we're finished with a
  map.

      void *map_get(MAP_T *m, const char *key);
  Returns a pointer to the value of the given key. If no
  mapping for the key exists then NULL will be returned.

      int map_set(MAP_T *m, const char *key, T value);
  Sets the given key to the given value. Returns 0 on success,
  otherwise -1 is returned and the map remains unchanged.

      void map_remove(MAP_T *m, const char *key);
  Removes the mapping of the given key from the map. If the key
  does not exist in the map then the function has no effect.
  Note: this function should not be called while iterating over
  the map.

      map_iter_t map_iter(MAP_T *m);
  Returns a map_iter_t which can be used with map_next() to
  iterate all the keys in the map.

      const char *map_next(MAP_T *m, map_iter_t *iter);
  Uses the map_iter_t returned by map_iter() to iterate all the
  keys in the map. map_next() returns a key with each call and
  returns NULL when there are no more keys.

  ### Predefined map types

      typedef map_t(void*) map_void_t;
      typedef map_t(char*) map_str_t;
      typedef map_t(int) map_int_t;
      typedef map_t(char) map_char_t;
      typedef map_t(float) map_float_t;
      typedef map_t(double) map_double_t;

  ### Example

      typedef map_t(unsigned int) map_uint_t;

      map_uint_t m;
      unsigned int *p;
      const char *key;
      map_iter_t iter;

      map_init(&m);
      map_set(&m, "testkey", 123);
      p = map_get(&m, "testkey");

      iter = map_iter(&m);
      while ((key = map_next(&m, &iter))) {
        printf("%s -> %u\n", key, *map_get(&m, key));
      }

      map_deinit(&m);
*/


#include <string.h>

#define MAP_VERSION "0.1.0"

struct map_node_t;
typedef struct map_node_t map_node_t;

typedef struct {
  map_node_t **buckets;
  unsigned nbuckets, nnodes;
} map_base_t;

typedef struct {
  unsigned bucketidx;
  map_node_t *node;
} map_iter_t;


/**
  Defines a new map type.
*/
#define map_t(T)\
  struct { map_base_t base; T *ref; T tmp; }

/**
  Initialises the map, this must be called before the map can be used.
 */
#define map_init(m)\
  memset(m, 0, sizeof(*(m)))

/**
  Deinitialises the map, freeing the memory the map allocated
  during use; this should be called when we're finished with a
  map.
 */
#define map_deinit(m)\
  map_deinit_(&(m)->base)

/**
  Returns a pointer to the value of the given key. If no
  mapping for the key exists then NULL will be returned.
*/
#define map_get(m, key)\
  ( (m)->ref = map_get_(&(m)->base, key) )

/**
  Sets the given key to the given value. Returns 0 on success,
  otherwise -1 is returned and the map remains unchanged.
*/
#define map_set(m, key, value)\
  ( (m)->tmp = (value),\
    map_set_(&(m)->base, key, &(m)->tmp, sizeof((m)->tmp)) )

/**
  Removes the mapping of the given key from the map. If the key
  does not exist in the map then the function has no effect.
*/
#define map_remove(m, key)\
  map_remove_(&(m)->base, key)

/**
  Returns a map_iter_t which can be used with map_next() to
  iterate all the keys in the map.
*/
#define map_iter(m)\
  map_iter_()

/**
  Uses the map_iter_t returned by map_iter() to iterate all the
  keys in the map. map_next() returns a key with each call and
  returns NULL when there are no more keys.
 */
#define map_next(m, iter)\
  map_next_(&(m)->base, iter)


/**
  @name Internal functions
  These functions are called via the macros.  Don't call them directly.
  @{
 */
void map_deinit_(map_base_t *m);
void *map_get_(const map_base_t *m, const char *key);
int map_set_(map_base_t *m, const char *key, void *value, int vsize);
void map_remove_(map_base_t *m, const char *key);
map_iter_t map_iter_(void);
const char *map_next_(map_base_t *m, map_iter_t *iter);
/** @} */


/** @cond private */
typedef map_t(void*) map_void_t;
typedef map_t(char*) map_str_t;
typedef map_t(int) map_int_t;
typedef map_t(char) map_char_t;
typedef map_t(float) map_float_t;
typedef map_t(double) map_double_t;
/** @endcond */



#endif
