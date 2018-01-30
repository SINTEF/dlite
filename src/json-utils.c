/* json-utils.c */


#include "json-utils.h"


char json_char_type(json_t *obj)
{
  json_type typ;
  if (obj == NULL) {
    return 'x';
  } else {
    typ = json_typeof(obj);
    switch (typ) {
      case JSON_OBJECT:
        return 'o';
      case JSON_ARRAY:
        return 'a';
      case JSON_STRING:
        return 's';
      case JSON_INTEGER:
        return 'i';
      case JSON_REAL:
        return 'r';
      case JSON_TRUE:
        return 'b';
      case JSON_FALSE:
        return 'b';
      case JSON_NULL:
        return 'n';
      default:
        return 'x';
    }
  }
}

char json_merge_type(char t1, char t2) {
  if (t1 == 'x')
    return t2;
  else if (t1 == t2)
    return t2;
  else if (t1 != t2) {
    if ((t1 == 'i') && (t2 == 'r'))
      return 'r';
    else if ((t1 == 'r') && (t2 == 'i'))
      return 'r';
    else
      return 'm';
  }
}

char json_array_type(json_t *obj)
{
  size_t i, size;
  char cur = 'x';
  char item_type = 'x';
  json_t *item;
  if (json_is_array(obj)) {
    size = json_array_size(obj);
    for(i=0; i < size; i++) {
      item = json_array_get(obj, i);
      cur = json_char_type(item);
      if (cur == 'a')
        item_type = json_array_type(item);
      else
        item_type = json_merge_type(item_type, cur);
      if (item_type == 'm')
        break;
    }
  }
  return item_type;
}

int _merge_size(int s1, int s2)
{
  if (s1 == -2)
    return s2;
  else if (s1 == s2)
    return s2;
  else
    return -1;
}

void _array_size(json_t *arr, int ndim, int *dims)
{
  int i, size;
  json_t *item;

  if (ndim >= NDIM_MAX)
    return;

  if (json_is_array(arr)) {
    //printf("array_size, dim:%i (%i, %i, %i)\n", ndim, dims[0], dims[1], dims[2]);
    size = (int)(json_array_size(arr));
    dims[ndim] = _merge_size(dims[ndim], size);
    for(i=0; i < size; i++) {
      item = json_array_get(arr, i);
      _array_size(item, ndim + 1, dims);
    }
  }
}

ivec_t *json_array_dimensions(json_t *obj)
{
  int dims[NDIM_MAX];
  int i, d;
  ivec_t *ans = NULL;

  for(i = 0; i < NDIM_MAX; i++)
    dims[i] = -2;

  _array_size(obj, 0, &dims);

  d = 0;
  for(i = 0; i < NDIM_MAX; i++) {
    if (dims[i] == -2) {
      break;
    } else if (dims[i] == -1) {
      d = -1;
      break;
    }
    else {
      d++;
    }
  }
  if (d > 0) {
    ans = ivec();
    for(i = 0; i < d; i++) {
      ivec_add(ans, dims[i]);
    }
  }
  return ans;
}

int json_to_int(json_t *obj) {
  if (json_is_integer(obj))
    return json_integer_value(obj);
  else if (json_is_true(obj))
    return 1;
  else if (json_is_false(obj))
    return 0;
  else if (json_is_real(obj))
    return (int)(json_real_value(obj));
  //else if (json_is_null(obj))
  //  return (int)(nan);
  else
    return 0;
}

void flatten_i(json_t *obj, ivec_t *arr)
{
  int i, size;
  json_t *item;

  if (json_is_array(obj)) {
    size = (int)(json_array_size(obj));
    for(i=0; i < size; i++) {
      item = json_array_get(obj, i);
      flatten_i(item, arr);
    }
  }
  else {
    ivec_add(arr, json_to_int(obj));
  }
}

ivec_t *json_array_flatten_i(json_t *obj)
{
  ivec_t *arr = NULL;
  if (json_is_array(obj)) {
    arr = ivec();
    flatten_i(obj, arr);
  }
  return arr;
}

double json_to_real(json_t *obj) {
  if (json_is_real(obj))
    return json_real_value(obj);
  else if (json_is_integer(obj))
    return (double)(json_integer_value(obj));
  else if (json_is_true(obj))
    return 1.0;
  else if (json_is_false(obj))
    return 0.0;
  //else if (json_is_null(obj))
  //  return (int)(nan);
  else
    return 0;
}

void flatten_r(json_t *obj, vec_t *arr)
{
  int i, size;
  json_t *item;

  if (json_is_array(obj)) {
    size = (int)(json_array_size(obj));
    for(i=0; i < size; i++) {
      item = json_array_get(obj, i);
      flatten_r(item, arr);
    }
  }
  else {
    vec_add(arr, json_to_real(obj));
  }
}

vec_t *json_array_flatten_r(json_t *obj)
{
  vec_t *arr = NULL;
  if (json_is_array(obj)) {
    arr = vec();
    flatten_r(obj, arr);
  }
  return arr;
}


json_data_t *json_data()
{
  json_data_t *d;
  d = (json_data_t*) malloc(sizeof(json_data_t));
  d->dtype = 'x';
  d->dims = NULL;
  d->scalar_i = 0;
  d->scalar_r = 0.0;
  d->scalar_s = NULL;
  d->array_i = NULL;
  d->array_r = NULL;
  return d;
}

void json_data_free(json_data_t *d)
{
  ivec_free(d->dims);
  ivec_free(d->array_i);
  vec_free(d->array_r);
  free(d);
}

json_data_t *json_get_data(json_t *obj)
{
  json_data_t *data = json_data();
  data->dtype = json_char_type(obj);
  int ok = 1;
  switch(data->dtype) {
  case 'a':
    data->dtype = json_array_type(obj);
    data->dims = json_array_dimensions(obj);
    if (data->dims != NULL) {
      if (data->dtype == 'i')
        data->array_i = json_array_flatten_i(obj);
      else if (data->dtype == 'r')
        data->array_r = json_array_flatten_r(obj);
      //else if (data->dtype == 's')
      //  data->array_s = json_array_flatten_s(obj);
      else {
        /* mixed data type */
        ok = 0;
      }
    }
    break;
  case 'i':
    data->scalar_i = json_integer_value(obj);
    break;
  case 'r':
    data->scalar_r = json_real_value(obj);
    break;
  case 'b':
    data->scalar_i = json_is_true(obj) ? 1 : 0;
    break;
  case 's':
    data->scalar_s = json_string_value(obj);
    break;
  case 'x':
    ok = 0;
    break;
  }
  if (ok == 0) {
    json_data_free(data);
    data = NULL;
  }
  return data;
}

