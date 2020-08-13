/* json-utils.c */

#include <assert.h>

#include "utils/err.h"
#include "utils/boolean.h"
#include "utils/floats.h"
#include "str.h"
#include "json-utils.h"
#include "dlite-macros.h"
#include "dlite-type.h"
#include "dlite-entity.h"


/* Returns the type of the json object as character:
 * x: undefined type
 * o: object
 * a: array
 * i: integer
 * r: real
 * s: string
 * b: boolean
 * n: null
 */
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

/* Combine the type an item (t1) with the next
 * item (t2) of a json array
 * return 'm' if the array contains different types
 * (e.g. the array contains a real and a string)
 */
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
  else
    return 'x';
}

/* Scan each item of the array and returns the type of the items:
 * i: the array contains only integer values
 * r: the array contains real values and/or integer values
 * s: the array contains only string values
 * m: the array contains different types (e.g. the array contains a real and a string)
 * x: undefined type
 */
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
    /*printf("array_size, dim:%i (%i, %i, %i)\n", ndim, dims[0], dims[1], dims[2]);*/
    size = (int)(json_array_size(arr));
    dims[ndim] = _merge_size(dims[ndim], size);
    for(i=0; i < size; i++) {
      item = json_array_get(arr, i);
      _array_size(item, ndim + 1, dims);
    }
  }
}

/* Return the shape (dimensions) of the json value:
 *  - NULL: the json value is a scalar (real, integer, string, or object)
 *  - valid pointer of ivec_t: the json value is an array
 */
ivec_t *json_array_dimensions(json_t *obj)
{
  int dims[NDIM_MAX];
  int i, d;
  ivec_t *ans = NULL;

  for(i = 0; i < NDIM_MAX; i++)
    dims[i] = -2;

  _array_size(obj, 0, &dims[0]);

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

/* Convert the JSON value to a integer */
int json_to_int(json_t *obj) {
  if (json_is_integer(obj))
    return json_integer_value(obj);
  else if (json_is_true(obj))
    return 1;
  else if (json_is_false(obj))
    return 0;
  else if (json_is_real(obj))
    return (int)(json_real_value(obj));
  /*
  else if (json_is_null(obj))
    return (int)(nan);
  */
  else
    return 0;
}

/* Recursive function to collapse an array of integer */
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

/* Return a copy of the json array collapsed
 * into one dimension (array of integer)
 */
ivec_t *json_array_flatten_i(json_t *obj)
{
  ivec_t *arr = NULL;
  if (json_is_array(obj)) {
    arr = ivec();
    flatten_i(obj, arr);
  }
  return arr;
}

/* Convert the JSON value to a real */
double json_to_real(json_t *obj) {
  if (json_is_real(obj))
    return json_real_value(obj);
  else if (json_is_integer(obj))
    return (double)(json_integer_value(obj));
  else if (json_is_true(obj))
    return 1.0;
  else if (json_is_false(obj))
    return 0.0;
  /*else if (json_is_null(obj))
      return (int)(nan);*/
  else
    return 0;
}

/* Recursive function to collapse an array of real */
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

/* Return a copy of the json array collapsed
 * into one dimension (array of real)
 */
vec_t *json_array_flatten_r(json_t *obj)
{
  vec_t *arr = NULL;
  if (json_is_array(obj)) {
    arr = vec();
    flatten_r(obj, arr);
  }
  return arr;
}

/* Recursive function to collapse an array of string */
void flatten_s(json_t *obj, str_list_t *arr)
{
  int i, size;
  json_t *item;

  if (json_is_array(obj)) {
    size = (int)(json_array_size(obj));
    for(i=0; i < size; i++) {
      item = json_array_get(obj, i);
      flatten_s(item, arr);
    }
  }
  else {
    str_list_add(arr, (char *)json_string_value(obj), true);
  }
}

/* Return a copy of the json array collapsed
 * into one dimension (array of string)
 */
str_list_t *json_array_flatten_s(json_t *obj)
{
  str_list_t *arr = NULL;
  if (json_is_array(obj)) {
    arr = str_list();
    flatten_s(obj, arr);
  }
  return arr;
}


json_data_t *json_data()
{
  json_data_t *d;
  d = (json_data_t*) calloc(1, sizeof(json_data_t));
  d->dtype = 'x';
  d->dims = NULL;
  d->array_i = NULL;
  d->array_r = NULL;
  d->array_s = NULL;
  return d;
}

void json_data_free(json_data_t *d)
{
  ivec_free(d->dims);
  ivec_free(d->array_i);
  vec_free(d->array_r);
  str_list_free(d->array_s, false);
  free(d);
}


json_data_t *json_get_data(json_t *obj)
{
  int ok;
  json_data_t *data;

  ok = 1;

  data = json_data();
  data->dtype = json_char_type(obj);

  switch(data->dtype) {
  case 'a':
    data->dtype = json_array_type(obj);
    data->dims = json_array_dimensions(obj);
    if (data->dims != NULL) {
      if (data->dtype == 'i')
        data->array_i = json_array_flatten_i(obj);
      else if (data->dtype == 'r')
        data->array_r = json_array_flatten_r(obj);
      else if (data->dtype == 's')
        data->array_s = json_array_flatten_s(obj);
      else {
        /* mixed data type */
        ok = 0;
      }
    }
    break;
  case 'i':
    data->array_i = ivec();
    ivec_add(data->array_i, json_integer_value(obj));
    break;
  case 'r':
    data->array_r = vec();
    vec_add(data->array_r, json_real_value(obj));
    break;
  case 'b':
    data->array_i = ivec();
    ivec_add(data->array_i, json_is_true(obj) ? 1 : 0);
    break;
  case 's':
    data->array_s = str_list();
    str_list_add(data->array_s, (char *)json_string_value(obj), true);
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

/* Create a json array from an array of integer */
json_t *json_array_int(ivec_t *data)
{
  json_t *arr = json_array();
  size_t i, size;
  size = ivec_size(data);
  for(i=0; i<size; i++)
    json_array_append(arr, json_integer(data->data[i]));
  return arr;
}

/* Create a json array from an array of real */
json_t *json_array_real(vec_t *data)
{
  json_t *arr = json_array();
  size_t i, size;
  size = vec_size(data);
  for(i=0; i<size; i++)
    json_array_append(arr, json_real(data->data[i]));
  return arr;
}

/* Create a json array from an array of boolean */
json_t *json_array_bool(ivec_t *data)
{
  json_t *arr = json_array();
  size_t i, size;
  size = ivec_size(data);
  for(i=0; i<size; i++)
    json_array_append(arr, data->data[i] ? json_true() : json_false());
  return arr;
}

/* Create a json array from an array of string */
json_t *json_array_string(str_list_t *data)
{
  json_t *arr = json_array();
  size_t i, size;
  size = str_list_size(data);
  for(i=0; i < size; i++)
    json_array_append(arr, json_string(data->data[i]));
  return arr;
}

int json_set_data(json_t *obj, const char *name, const json_data_t *data)
{
  json_t *value;
  if (json_is_object(obj) && data && (!str_is_whitespace(name))) {
    switch (data->dtype) {
    case 'i':
      if (ivec_size(data->dims) > 0)
        value = json_array_int(data->array_i);
      else if (ivec_size(data->array_i) > 0)
        value = json_integer(data->array_i->data[0]);
      break;
    case 'r':
      if (ivec_size(data->dims) > 0)
        value = json_array_real(data->array_r);
      else if (vec_size(data->array_r) > 0)
        value = json_real(data->array_r->data[0]);
      break;
    case 'b':
      if (ivec_size(data->dims) > 0)
        value = json_array_bool(data->array_i);
      else if (ivec_size(data->array_i) > 0)
        value = data->array_i->data[0] ? json_true() : json_false();
      break;
    case 's':
      if (ivec_size(data->dims) > 0)
        value = json_array_string(data->array_s);
      else if (str_list_size(data->array_s) > 0)
        value = json_string(data->array_s->data[0]);
      break;
    default:
      value = json_null();
      break;
    }
    json_object_set(obj, name, value);
    return 0;
  }
  return 1;
}

int check_dimensions(const char *prop_name, json_t *prop_dims,
                     json_t *entity_dims)
{
  size_t i, j, sp, se, k, found;
  json_t *e;
  json_t *p;
  json_t *dname;

  k = 0;
  sp = json_array_size(prop_dims);
  se = json_array_size(entity_dims);
  for(i = 0; i < sp; i++) {
    p = json_array_get(prop_dims, i);
    found = 0;
    for(j = 0; j < se; j++) {
      e = json_array_get(entity_dims, j);
      dname = json_object_get(e, "name");
      if (str_equal(json_string_value(p), json_string_value(dname))) {
        k++;
        found++;
      }
    }
    if (found == 0)
      printf("error: the dimension \"%s\" of the property \"%s\" is not defined.\n", json_string_value(p), prop_name);
  }
  return (int)(k == sp);
}


int dlite_json_entity_dim_count(json_t *obj)
{
  int count = 0;
  int nerr = 0;
  size_t i, size;
  json_t *dims;
  json_t *item;
  json_t *name;

  if (json_is_object(obj)) {
    dims = json_object_get(obj, "dimensions");
    if (json_is_array(dims)) {
      size = json_array_size(dims);
      for (i = 0; i < size; i++) {
        item = json_array_get(dims, i);
        name = json_object_get(item, "name");
        if (str_is_whitespace(json_string_value(name))) {
          printf("error: the dimension [%d] has not a valid name.\n", (int)(i + 1));
          nerr++;
        }
        else
          count++;
      }
    }
  }
  return nerr > 0 ? -1 : count;
}


int dlite_json_entity_prop_count(json_t *obj)
{
  int count = 0;
  int nerr = 0;
  size_t i, size;
  json_t *dims;
  json_t *props;
  json_t *item;
  json_t *name;
  json_t *ptype;

  if (json_is_object(obj)) {
    dims = json_object_get(obj, "dimensions");
    props = json_object_get(obj, "properties");
    if (json_is_array(props)) {
      size = json_array_size(props);
      for (i = 0; i < size; i++) {
        item = json_array_get(props, i);
        name = json_object_get(item, "name");
        ptype = json_object_get(item, "type");
        if (str_is_whitespace(json_string_value(name))) {
          printf("error: the property [%d] has not a valid name.\n",
                 (int)(i + 1));
          nerr++;
        }
        else if (!dlite_is_type(json_string_value(ptype))) {
          printf("error: the property [%d] \"%s\" has not a valid type.\n",
                 (int)(i + 1), json_string_value(name));
          nerr++;
        }
        else if (!check_dimensions(json_string_value(name),
                                   json_object_get(item, "dims"), dims)) {
          printf("error: the dimension of the property \"%s\" are not well "
                 "defined.\n", json_string_value(name));
          nerr++;
        }
        else
          count++;
      }
    }
  }
  return nerr > 0 ? -1 : count;
}


/* Returns a json string containing the encoded binary blob `src` of
   length `n`.  Returns NULL on error. */
json_t *hex_encode(const uint8_t *src, size_t n)
{
  size_t i;
  char *dest = malloc(2*n+1);
  json_t *v=NULL;
  for (i=0; i<n; i++)
    if (snprintf(dest + 2*i, 3, "%02hhx", src[i]) != 2)
      FAIL1("invalid hex string: '%s'", src);
  v = json_string(dest);
 fail:
  free(dest);
  return v;
}


/* Decode hex string `src` and write the result to `dest`.  `n` is the
   length of `dest`.  Returns non-zero on error. */
int hex_decode(uint8_t *dest, const json_t *src, size_t n)
{
  size_t i;
  const char *s;
  if (!json_is_string(src)) return errx(1, "expected json string");
  if (json_string_length(src) != 2*n)
    return errx(1, "expected encoded blob length: %lu, got %lu",
                2*n, json_string_length(src));
  s = json_string_value(src);
  for (i=0; i<n; i++)
    if (sscanf(s + 2*i, "%2hhx", dest + i) != 1)
      return err(-1, "invalid hex string: '%s'", s);
  return 0;
}


/* Returns a new json item with the data at `ptr` (which has type `type` and
 * size `size`).  Returns NULL on error.
*/
json_t *dlite_json_set_value(const void *ptr, DLiteType type, size_t size,
                             const json_t *root)
{
  bool bval;
  json_int_t ival;
  double fval;
  char *sval;

  switch (type) {

  case dliteBlob:
    return hex_encode(ptr, size);

  case dliteBool:
    assert(size == sizeof(bool));
    bval = *((bool *)ptr);
    return json_boolean(bval);

  case dliteInt:
    switch (size) {
    case 1:  ival = *((int8_t *)ptr);  break;
    case 2:  ival = *((int16_t *)ptr); break;
    case 4:  ival = *((int32_t *)ptr); break;
    case 8:  ival = *((int64_t *)ptr); break;
    default: return errx(-1, "invalid int size: %lu", size), NULL;
    }
    return json_integer(ival);

  case dliteUInt:
    switch (size) {
    case 1:  ival = *((uint8_t *)ptr);  break;
    case 2:  ival = *((uint16_t *)ptr); break;
    case 4:  ival = *((uint32_t *)ptr); break;
    case 8:  ival = *((uint64_t *)ptr); break;
    default: return errx(-1, "invalid uint size: %lu", size), NULL;
    }
    return json_integer(ival);

  case dliteFloat:
    switch (size) {
    case  4: fval = *((float32_t *)ptr);  break;
    case  8: fval = *((float64_t *)ptr);  break;
#ifdef HAVE_FLOAT80
    case 10: fval = *((float80_t *)ptr);  break;
#endif
#ifdef HAVE_FLOAT128
    case 16: fval = *((float128_t *)ptr); break;
#endif
    default: return errx(-1, "invalid float size: %lu", size), NULL;
    }
    return json_real(fval);

  case dliteFixString: {
    size_t len = strlen((char *)ptr);
    sval = (char *)ptr;
    return json_stringn(sval, (len < size) ? len : size);
  }

  case dliteStringPtr:
    sval = *((char **)ptr);
    return (sval) ? json_string(sval) : json_null();

  case dliteDimension: {
    const DLiteDimension *d = ptr;
    json_t *obj = json_object();
    json_object_set_new(obj, "name", json_string(d->name));
    json_object_set_new(obj, "description", json_string(d->description));
    return obj;
  }

  case dliteProperty: {
    const DLiteProperty *p = ptr;
    char typename[30];
    json_t *obj = json_object();
    dlite_type_set_typename(p->type, p->size, typename, sizeof(typename));

    json_object_set_new(obj, "name", json_string(p->name));
    json_object_set_new(obj, "type", json_string(typename));
    if (p->ndims) {
      int i;
      json_t *arr = json_array();
      json_t *dimensions = json_object_get(root, "dimensions");
      if (!json_is_array(dimensions)) {
        json_t *properties = json_object_get(root, "properties");
        dimensions = json_object_get(properties, "dimensions");
      }
      if (!dimensions)
        return errx(-1, "JSON storage: dimensions must be set before "
                    "properties"), NULL;
      if (!json_is_array(dimensions))
        return errx(-1, "JSON storage: dimensions should be an array"), NULL;
      for (i=0; i<p->ndims; i++) {
        json_t *dimname = json_string(p->dims[i]);
        json_array_append_new(arr, dimname);
      }
      json_object_set_new(obj, "dims", arr);
    }
    if (p->unit && *p->unit)
      json_object_set_new(obj, "unit", json_string(p->unit));
    if (p->iri && *p->iri)
      json_object_set_new(obj, "iri", json_string(p->iri));
    if (p->description)
      json_object_set_new(obj, "description", json_string(p->description));
    return obj;
  }

  case dliteRelation: {
    const DLiteRelation *r = ptr;
    json_t *obj = json_object();
    json_object_set_new(obj, "s", json_string(r->s));
    json_object_set_new(obj, "p", json_string(r->p));
    json_object_set_new(obj, "o", json_string(r->o));
    if (r->id) json_object_set_new(obj, "id", json_string(r->id));
    return obj;
  }

  default:
    return errx(-1, "JSON storage, unsupported type number: %d", type), NULL;
  }
  assert(0);  /* should never be reached */
}


int parse_property(void *ptr, const json_t *item, const json_t *root);




/* Copies the value of JSON item `item` to memory pointed to by `ptr`
   (which should be large enough to hold `size` bytes).  `type` and
   `size` is the destination type and size.  Returns non-zero on error.

   The `jdims` should be the json root for dimensions.  It allow properties
   to inspect dimension names.
*/
int dlite_json_get_value(void *ptr, const json_t *item,
                         DLiteType type, size_t size,
                         const json_t *root)
{
  DLiteDimension dimension;
  DLiteRelation relation;
  json_t *str;
  const char *s;

  switch (type) {
  case dliteBlob:
    if (hex_decode(ptr, item, size)) return 1;
    break;

  case dliteBool:
    if (!json_is_boolean(item)) return errx(1, "expected json boolean");
    *((bool *)ptr) = json_boolean_value(item);
    break;

  case dliteInt:
    if (!json_is_integer(item)) return errx(1, "expected json integer");
    switch (size) {
    case 1:  *((int8_t *)ptr)  = json_integer_value(item);  break;
    case 2:  *((int16_t *)ptr) = json_integer_value(item);  break;
    case 4:  *((int32_t *)ptr) = json_integer_value(item);  break;
    case 8:  *((int64_t *)ptr) = json_integer_value(item);  break;
    default: return errx(1, "invalid int size: %lu", size);
    }
    break;

  case dliteUInt:
    if (!json_is_integer(item)) return errx(1, "expected json integer");
    if (json_integer_value(item) < 0) return errx(1, "expected unsigned int");
    switch (size) {
    case 1:  *((uint8_t *)ptr)  = json_integer_value(item);  break;
    case 2:  *((uint16_t *)ptr) = json_integer_value(item);  break;
    case 4:  *((uint32_t *)ptr) = json_integer_value(item);  break;
    case 8:  *((uint64_t *)ptr) = json_integer_value(item);  break;
    default: return errx(1, "invalid uint size: %lu", size);
    }
    break;

  case dliteFloat:
    if (!json_is_real(item)) return errx(1, "expected json real");
    switch (size) {
    case  4: *((float32_t *)ptr)  = (float32_t)json_real_value(item);  break;
    case  8: *((float64_t *)ptr)  = json_real_value(item);  break;
#ifdef HAVE_FLOAT80
    case 10: *((float80_t *)ptr)  = json_real_value(item);  break;
#endif
#ifdef HAVE_FLOAT128
    case 16: *((float128_t *)ptr) = json_real_value(item);  break;
#endif
    default: return errx(1, "invalid float size: %lu", size);
    }
    break;

  case dliteFixString:
    if (!json_is_string(item)) return errx(1, "expected json string");
    if (json_string_length(item) > size)
      return errx(1, "length of JSON string (%lu), exceeds buffer size (%lu)",
                  json_string_length(item), size);
    strncpy(ptr, json_string_value(item), size);
    if (size > 0) ((char *)ptr)[size-1] = '\0';
    break;

  case dliteStringPtr:
    if (json_is_null(item)) {
      *((void **)ptr) = NULL;
    } else if (json_is_string(item)) {
      if (!(*((char **)ptr) = strdup(json_string_value(item))))
        return errx(1, "allocation error");
    } else {
      return errx(1, "expected json string");
    }
    break;

  case dliteDimension:
    memset(&dimension, 0, sizeof(DLiteDimension));

    if (!json_is_object(item))
      FAIL("expected json dimension object");

    if (!(str = json_object_get(item, "name")) ||
        !(s = json_string_value(str)))
      FAIL("expected json object with dimension name");
    if (!(dimension.name = strdup(s))) FAIL("allocation failure");

    if (!(str = json_object_get(item, "description")) ||
        !(s = json_string_value(str)))
      FAIL("expected json object with dimension description");
    if (!(dimension.description = strdup(s))) FAIL("allocation failure");

    memcpy(ptr, &dimension, sizeof(dimension));
    break;

  case dliteProperty:
    if (parse_property(ptr, item, root)) goto fail;
    break;

  case dliteRelation:
    memset(&relation, 0, sizeof(relation));
    if (!json_is_object(item)) FAIL("expected json relation object");

    if (!(str = json_object_get(item, "s")) ||
        !(s = json_string_value(str)))
      FAIL("expected relation subject (s)");
    if (!(relation.s = strdup(s))) FAIL("allocation failure");

    if (!(str = json_object_get(item, "p")) ||
        !(s = json_string_value(str)))
      FAIL("expected relation predicate (p)");
    if (!(relation.p = strdup(s))) FAIL("allocation failure");

    if (!(str = json_object_get(item, "o")) ||
        !(s = json_string_value(str)))
      FAIL("expected relation object (o)");
    if (!(relation.o = strdup(s))) FAIL("allocation failure");

    if ((str = json_object_get(item, "id")) &&
        (s = json_string_value(str)) &&
        !(relation.id = strdup(s))) FAIL("allocation failure");

    memcpy(ptr, &relation, sizeof(relation));
    break;

  default:
    return errx(1, "reading JSON data of type '%s' is not yet supported",
                dlite_type_get_dtypename(type));
  }
  return 0;

 fail:
  return 1;
}


/* help function for dlite_json_get_value() */
int parse_property(void *ptr, const json_t *item, const json_t *root)
{
  DLiteProperty property;
  json_t *str, *arr;
  const char *s;
  char **dims=NULL;
  int j;
  int ndims=0;

  UNUSED(root);
  memset(&property, 0, sizeof(DLiteProperty));

  if (!json_is_object(item)) FAIL("expected json property object");

  if (!(str = json_object_get(item, "name")) || !(s = json_string_value(str)))
    FAIL("expected property name");
  if (!(property.name = strdup(s))) FAIL("allocation failure");

  if (!(str = json_object_get(item, "type")) || !(s = json_string_value(str)))
    FAIL("expected property type");
  if (dlite_type_set_dtype_and_size(s, &property.type, &property.size))
    goto fail;

  if ((arr = json_object_get(item, "dims"))) {
    if (!json_is_array(arr)) FAIL("expected 'dims' to be a json array");
    ndims = json_array_size(arr);
    if (!(dims = calloc(ndims, sizeof(char *)))) FAIL("allocation failure");

    for (j=0; j<ndims; j++) {
      const char *s;
      json_t *jdim  = json_array_get(arr, j);
      assert(jdim);
      if (!(s = json_string_value(jdim)))
        FAIL("expected property dimension elements to be strings");
      dims[j] = strdup(s);
    }
    property.ndims = ndims;
    property.dims = dims;
  }

  if ((str = json_object_get(item, "unit"))) {
    if (!(s = json_string_value(str))) FAIL("unit must be a string");
    if (!(property.unit = strdup(s))) FAIL("allocation failure");
  }

  if ((str = json_object_get(item, "iri"))) {
    if (!(s = json_string_value(str))) FAIL("iri must be a string");
    if (!(property.iri = strdup(s))) FAIL("allocation failure");
  }

  if ((str = json_object_get(item, "description"))) {
    if (!(s = json_string_value(str))) FAIL("description must be a string");
    if (!(property.description = strdup(s))) FAIL("allocation failure");
  }

  /*
  printf("\n");
  printf("--- name: '%s'\n", property.name);
  printf("--- type: %d\n", property.type);
  printf("--- size: %lu\n", property.size);
  printf("--- ndims: %d\n", property.ndims);
  printf("--- dims: ");
  for (i=0; i<ndims; i++) printf("  %d", dims[i]);
  printf("\n");
  printf("--- unit: '%s'\n", property.unit);
  printf("--- iri: '%s'\n", property.iri);
  printf("--- description: '%s'\n", property.description);
  */

  memcpy(ptr, &property, sizeof(property));
  return 0;
 fail:
  if (dims) {
    for (j=0; j<ndims; j++)
      if (dims[j]) free(dims[j]);
    free(dims);
  }
  return 1;
}
