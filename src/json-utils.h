/* json-utils.h */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <jansson.h>


#define NDIM_MAX 50

char json_char_type(json_t *obj);
char json_array_type(json_t *obj);
void json_array_dimensions(json_t *obj, int *ndim, int *dimensions);
