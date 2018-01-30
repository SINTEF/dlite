/* json-utils.h */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <jansson.h>
#include "vector.h"

#define NDIM_MAX 50

typedef struct {
  /* Type of the data: i, r, b, s */
  char dtype;
  /* Dimensions of the data (=NULL if data is a scalar) */
  ivec_t *dims;
  /* Scalar data for integer (dtype=i) and boolean (dtype=b) */
  int scalar_i;
  /* Scalar data for real (dtype=r) */
  double scalar_r;
  /* Scalar data for string (dtype=s) */
  char* scalar_s;
  /* Array data for integer (dtype=i and dims!=NULL) */
  ivec_t *array_i;
  /* Array data for real (dtype=r and dims!=NULL) */
  vec_t *array_r;
} json_data_t;

char json_char_type(json_t *obj);
char json_array_type(json_t *obj);
ivec_t *json_array_dimensions(json_t *obj);

json_data_t *json_data();
json_data_t *json_get_data(json_t *obj);
void json_data_free();

