/* str.h */

#ifndef STR_H
#define STR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "boolean.h"

bool str_is_null(const char* s);
bool str_is_empty(const char* s);
bool str_is_whitespace(const char* s);

size_t str_size(const char* s);
char *str_copy(const char* s);

bool str_equal(const char* a, const char* b);

#endif /* STR_H */
