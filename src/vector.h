/* vector.h */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  int *data;
  size_t capacity;
  size_t size;
} ivec_t;

typedef struct {
  double *data;
  size_t capacity;
  size_t size;
} vec_t;


ivec_t *ivec();
size_t ivec_size(ivec_t *v);
void ivec_fill(ivec_t *v, int value);
void ivec_add(ivec_t *v, int value);
void ivec_resize(ivec_t *v, size_t size);
void ivec_reserve(ivec_t *v, size_t capacity);
void ivec_free(ivec_t *v);
void ivec_print(ivec_t *v, char* name);

vec_t *vec();
size_t vec_size(vec_t *v);
void vec_fill(vec_t *v, double value);
void vec_add(vec_t *v, double value);
void vec_resize(vec_t *v, size_t size);
void vec_reserve(vec_t *v, size_t capacity);
void vec_free(vec_t *v);
void vec_print(vec_t *v, char* name);
