/* json-utils.h */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <jansson.h>
#include "vector.h"
#include "str.h"

#define NDIM_MAX 50

typedef struct {
  /* Type of the data: i, r, b, s */
  char dtype;
  /* Dimensions of the data (=NULL if data is a scalar) */
  ivec_t *dims;
  /* Array data for integer (dtype==i and dtype==b) */
  ivec_t *array_i;
  /* Array data for real (dtype==r) */
  vec_t *array_r;
  /* Array data for real (dtype==s) */
  str_list_t *array_s;
} json_data_t;

char json_char_type(json_t *obj);
char json_array_type(json_t *obj);
ivec_t *json_array_dimensions(json_t *obj);

json_data_t *json_data();
json_data_t *json_get_data(json_t *obj);
int json_set_data(json_t *obj, const char *name, const json_data_t *data);
void json_data_free(json_data_t *d);

/* Return true (=1) if each dimension of the property is
   defined in the list of dimension of the entity
 */
int check_dimensions(char *prop_name, json_t *prop_dims, json_t *entity_dims);

/* Count the number of valid dimensions in the json object.
 * a dimension must have a name (not null, not empty, not only white space)
 */
int dlite_json_entity_dim_count(json_t *obj);

/* Count the number of valid properties in the json object.
 * a property must have a name and a valid type.
 */
int dlite_json_entity_prop_count(json_t *obj);
