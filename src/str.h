/* str.h */

#ifndef STR_H
#define STR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "boolean.h"

typedef struct {
  char **data;
  size_t capacity;
  size_t size;
} str_list_t;

bool str_is_null(const char* s);
bool str_is_empty(const char* s);
bool str_is_whitespace(const char* s);

size_t str_size(const char* s);
char *str_copy(const char* s);

bool str_equal(const char* a, const char* b);

str_list_t *str_list();
str_list_t *str_list1(char *x, bool copy);
str_list_t *str_list2(char *x, char *y, bool copy);

size_t str_list_size(str_list_t *v);
void str_list_add(str_list_t *v, char *value, bool copy);
void str_list_resize(str_list_t *v, size_t size);
void str_list_reserve(str_list_t *v, size_t capacity);
void str_list_free(str_list_t *v, bool free_items);
void str_list_print(str_list_t *v, char* name);


#endif /* STR_H */
