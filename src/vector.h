
/* vector.h */

#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dlite-type.h"

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
ivec_t *ivec1(int x);
ivec_t *ivec2(int x, int y);
ivec_t *ivec3(int x, int y, int z);

size_t ivec_size(ivec_t *v);
void ivec_fill(ivec_t *v, int value);
void ivec_add(ivec_t *v, int value);
void ivec_resize(ivec_t *v, size_t size);
void ivec_reserve(ivec_t *v, size_t capacity);
void ivec_free(ivec_t *v);
void ivec_print(ivec_t *v, char* name);
int ivec_cumprod(ivec_t *v);
int ivec_cumsum(ivec_t *v);

void ivec_copy_cast(ivec_t *v, DLiteType dtype, size_t dsize, void *dst);
ivec_t *ivec_create(DLiteType dtype, size_t dsize, size_t num, const void *src);

vec_t *vec();
vec_t *vec1(double x);
vec_t *vec2(double x, double y);
vec_t *vec3(double x, double y, double z);
vec_t *vecn(size_t n, double init);

size_t vec_size(vec_t *v);
void vec_fill(vec_t *v, double value);
void vec_add(vec_t *v, double value);
void vec_resize(vec_t *v, size_t size);
void vec_reserve(vec_t *v, size_t capacity);
void vec_free(vec_t *v);
void vec_print(vec_t *v, char* name);

void vec_copy_cast(vec_t *v, DLiteType dtype, size_t dsize, void *dst);
vec_t *vec_create(DLiteType dtype, size_t dsize, size_t num, const void *src);


#endif /* VECTOR_H */
