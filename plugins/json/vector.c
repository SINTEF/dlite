
#include "vector.h"
#include "boolean.h"
#include "integers.h"
#include "floats.h"

ivec_t *ivec()
{
  ivec_t *v;
  v = (ivec_t*) malloc(sizeof(ivec_t));
  v->capacity = 0;
  v->size = 0;
  v->data = NULL;
  return v;
}

ivec_t *ivec1(int x)
{
  ivec_t *v = ivec();
  ivec_reserve(v, 1);
  ivec_add(v, x);
  return v;
}

ivec_t *ivec2(int x, int y)
{
  ivec_t *v = ivec();
  ivec_reserve(v, 2);
  ivec_add(v, x);
  ivec_add(v, y);
  return v;
}

ivec_t *ivec3(int x, int y, int z)
{
  ivec_t *v = ivec();
  ivec_reserve(v, 3);
  ivec_add(v, x);
  ivec_add(v, y);
  ivec_add(v, z);
  return v;
}

ivec_t *ivecn(size_t n, int init)
{
  ivec_t *v = ivec();
  ivec_resize(v, n);
  ivec_fill(v, init);
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
  return v ? v->size : 0;
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

int ivec_cumprod(ivec_t *v)
{
  int p = 0;
  size_t i;
  size_t size = ivec_size(v);
  if (size > 0) {
    p = 1;
    for(i=0; i < size; i++)
      p *= v->data[i];
  }
  return p;
}

int ivec_cumsum(ivec_t *v)
{
  int s = 0;
  size_t i;
  size_t size = ivec_size(v);
  if (size > 0) {
    for(i=0; i < size; i++)
      s += v->data[i];
  }
  return s;
}

void ivec_copy_cast(ivec_t *v, DLiteType dtype, size_t dsize, void *dst)
{
  size_t vsize = ivec_size(v);
  size_t i=0;

  int *dst0;
  int8_t *dst1;
  int16_t *dst2;
  int32_t *dst4;
  int64_t *dst8;

  unsigned int *dst0u;
  uint8_t *dst1u;
  uint16_t *dst2u;
  uint32_t *dst4u;
  uint64_t *dst8u;

  bool *dstb;

  if (vsize > 0) {
    if (dtype == dliteInt) {
      if (dsize == sizeof(int)) {
        dst0 = (int*)dst;
        for(i=0; i < vsize; i++)
          dst0[i] = v->data[i];
      }
      else if (dsize == 1) {
        dst1 = (int8_t*)dst;
        for(i=0; i < vsize; i++)
          dst1[i] = (int8_t)(v->data[i]);
      }
      else if (dsize == 2) {
        dst2 = (int16_t*)dst;
        for(i=0; i < vsize; i++)
          dst2[i] = (int16_t)(v->data[i]);
      }
      else if (dsize == 4) {
        dst4 = (int32_t*)dst;
        for(i=0; i < vsize; i++)
          dst4[i] = (int32_t)(v->data[i]);
      }
      else if (dsize == 8) {
        dst8 = (int64_t*)dst;
        for(i=0; i < vsize; i++)
          dst8[i] = (int64_t)(v->data[i]);
      }
    }
    else if (dtype == dliteUInt) {
      if (dsize == sizeof(unsigned int)) {
        dst0u = (unsigned int*)dst;
        for(i=0; i < vsize; i++)
          dst0u[i] = (unsigned int)(v->data[i]);
      }
      else if (dsize == 1) {
        dst1u = (uint8_t*)dst;
        for(i=0; i < vsize; i++)
          dst1u[i] = (uint8_t)(v->data[i]);
      }
      else if (dsize == 2) {
        dst2u = (uint16_t*)dst;
        for(i=0; i < vsize; i++)
          dst2u[i] = (uint16_t)(v->data[i]);
      }
      else if (dsize == 4) {
        dst4u = (uint32_t*)dst;
        for(i=0; i < vsize; i++)
          dst4u[i] = (uint32_t)(v->data[i]);
      }
      else if (dsize == 8) {
        dst8u = (uint64_t*)dst;
        for(i=0; i < vsize; i++)
          dst8u[i] = (uint64_t)(v->data[i]);
      }
    }
    else if (dtype == dliteBool) {
      dstb = (bool*)dst;
      for(i=0; i < vsize; i++)
        dstb[i] = (bool)(v->data[i]);
    }
  }
}

ivec_t *ivec_create(DLiteType dtype, size_t dsize, size_t num, const void *src)
{
  ivec_t *v = NULL;
  size_t i;

  int *src0;
  int8_t *src1;
  int16_t *src2;
  int32_t *src4;
  int64_t *src8;

  unsigned int *src0u;
  uint8_t *src1u;
  uint16_t *src2u;
  uint32_t *src4u;
  uint64_t *src8u;

  bool *srcb;

  if (num > 0) {

    v = ivec();
    ivec_resize(v, num);

    if (dtype == dliteInt) {
      if (dsize == sizeof(int)) {
        src0 = (int*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src0[i];
      }
      else if (dsize == 1) {
        src1 = (int8_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src1[i];
      }
      else if (dsize == 2) {
        src2 = (int16_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src2[i];
      }
      else if (dsize == 4) {
        src4 = (int32_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src4[i];
      }
      else if (dsize == 8) {
        src8 = (int64_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src8[i];
      }
    }
    else if (dtype == dliteUInt) {
      if (dsize == sizeof(unsigned int)) {
        src0u = (unsigned int*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src0u[i];
      }
      else if (dsize == 1) {
        src1u = (uint8_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src1u[i];
      }
      else if (dsize == 2) {
        src2u = (uint16_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src2u[i];
      }
      else if (dsize == 4) {
        src4u = (uint32_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src4u[i];
      }
      else if (dsize == 8) {
        src8u = (uint64_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (int)src8u[i];
      }
    }
    else if (dtype == dliteBool) {
      srcb = (bool*)src;
      for(i=0; i < num; i++)
        v->data[i] = (int)srcb[i];
    }
  }
  return v;
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


vec_t *vec1(double x)
{
  vec_t *v = vec();
  vec_reserve(v, 1);
  vec_add(v, x);
  return v;
}

vec_t *vec2(double x, double y)
{
  vec_t *v = vec();
  vec_reserve(v, 2);
  vec_add(v, x);
  vec_add(v, y);
  return v;
}

vec_t *vec3(double x, double y, double z)
{
  vec_t *v = vec();
  vec_reserve(v, 3);
  vec_add(v, x);
  vec_add(v, y);
  vec_add(v, z);
  return v;
}

vec_t *vecn(size_t n, double init)
{
  vec_t *v = vec();
  vec_resize(v, n);
  vec_fill(v, init);
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
  return v ? v->size : 0;
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

void vec_copy_cast(vec_t *v, DLiteType dtype, size_t dsize, void *dst)
{
  size_t vsize = vec_size(v);
  size_t i=0;

  float *dstf;
  double *dstd;
  float32_t *dst4;
  float64_t *dst8;

  if (vsize > 0) {
    if (dtype == dliteFloat) {
      if (dsize == sizeof(float)) {
        dstf = (float*)dst;
        for(i=0; i < vsize; i++)
          dstf[i] = (float)(v->data[i]);
      }
      else if (dsize == sizeof(double)) {
        dstd = (double*)dst;
        for(i=0; i < vsize; i++)
          dstd[i] = v->data[i];
      }
      else if (dsize == 4) {
        dst4 = (float32_t*)dst;
        for(i=0; i < vsize; i++)
          dst4[i] = (float32_t)(v->data[i]);
      }
      else if (dsize == 8) {
        dst8 = (float64_t*)dst;
        for(i=0; i < vsize; i++)
          dst8[i] = (float64_t)(v->data[i]);
      }
    }
  }
}

vec_t *vec_create(DLiteType dtype, size_t dsize, size_t num, const void *src)
{
  vec_t *v = NULL;
  size_t i;

  float *srcf;
  double *srcd;
  float32_t *src4;
  float64_t *src8;

  if (num > 0) {
    v = vec();
    vec_resize(v, num);

    if (dtype == dliteFloat) {
      if (dsize == sizeof(float)) {
        srcf = (float*)src;
        for(i=0; i < num; i++)
          v->data[i] = (double)srcf[i];
      }
      else if (dsize == sizeof(double)) {
        srcd = (double*)src;
        for(i=0; i < num; i++)
          v->data[i] = (double)srcd[i];
      }
      else if (dsize == 4) {
        src4 = (float32_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (double)src4[i];
      }
      else if (dsize == 8) {
        src8 = (float64_t*)src;
        for(i=0; i < num; i++)
          v->data[i] = (double)src8[i];
      }
    }
  }
  return v;
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
