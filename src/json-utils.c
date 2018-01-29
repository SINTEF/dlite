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

void json_array_dimensions(json_t *obj, int *ndim, int *dimensions)
{
  int dims[NDIM_MAX];
  int i, d;

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
  for(i = 0; i < d; i++) {
    dimensions[i] = dims[i];
  }
  *ndim = d;
}

