
#include "vector.h"

ivec_t *ivec()
{
  ivec_t *v;
  v = (ivec_t*) malloc(sizeof(ivec_t));
  v->capacity = 0;
  v->size = 0;
  v->data = NULL;
  return v;
}

void ivec_fill(ivec_t *v, int value)
{
  size_t i;
  for(i=0; i < v->size; i++)
    v->data[i] = value;
}

void ivec_add(ivec_t *v, int value)
{
  if (v->size >= v->capacity)
    ivec_reserve(v, 2 * v->size);
  v->data[v->size] = value;
  v->size++;
}

size_t ivec_size(ivec_t *v)
{
  return v->size;
}

void ivec_resize(ivec_t *v, size_t size)
{
  v->size = size;
  if (v->size >= v->capacity)
    ivec_reserve(v, 2 * v->size);
}

void ivec_reserve(ivec_t *v, size_t capacity)
{
  if (capacity == 0)
    capacity = 10;
  if (v->capacity < capacity) {
    if (v->data == NULL)
      v->data = (int *)malloc(capacity * sizeof(int));
    else
      v->data = (int *)realloc(v->data, capacity * sizeof(int));
    v->capacity = capacity;
  }
}

void ivec_free(ivec_t *v)
{
  if (v) {
    if (v->data)
      free(v->data);
    free(v);
  }
}

void ivec_print(ivec_t *v, char* name)
{
  size_t i, last;
  if (v == NULL)
    printf("%s = NULL\n", name);
  else {
    printf("%s = [", name);
    last = v->size - 1;
    for(i=0; i < v->size; i++)
      if (i == last)
        printf("%d", v->data[i]);
      else
        printf("%d, ", v->data[i]);
    printf("]%c", '\n');
  }
}

/* vec_t - vector of double */

vec_t *vec()
{
  vec_t *v;
  v = (vec_t*) malloc(sizeof(vec_t));
  v->capacity = 0;
  v->size = 0;
  v->data = NULL;
  return v;
}

void vec_fill(vec_t *v, double value)
{
  size_t i;
  for(i=0; i < v->size; i++)
    v->data[i] = value;
}

void vec_add(vec_t *v, double value)
{
  if (v->size >= v->capacity)
    vec_reserve(v, 2 * v->size);
  v->data[v->size] = value;
  v->size++;
}

size_t vec_size(vec_t *v)
{
  return v->size;
}

void vec_resize(vec_t *v, size_t size)
{
  v->size = size;
  if (v->size >= v->capacity)
    vec_reserve(v, 2 * v->size);
}

void vec_reserve(vec_t *v, size_t capacity)
{
  if (capacity == 0)
    capacity = 10;
  if (v->capacity < capacity) {
    if (v->data == NULL)
      v->data = (double *)malloc(capacity * sizeof(double));
    else
      v->data = (double *)realloc(v->data, capacity * sizeof(double));
    v->capacity = capacity;
  }
}

void vec_free(vec_t *v)
{
  if (v) {
    if (v->data)
      free(v->data);
    free(v);
  }
}

void vec_print(vec_t *v, char* name)
{
  size_t i, last;
  if (v == NULL)
    printf("%s = NULL\n", name);
  else {
    printf("%s = [", name);
    last = v->size - 1;
    for(i=0; i < v->size; i++)
      if (i == last)
        printf("%f", v->data[i]);
      else
        printf("%f, ", v->data[i]);
    printf("]%c", '\n');
  }
}
