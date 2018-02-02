
#include "str.h"

bool str_is_null(const char* s)
{
  return s == NULL;
}

bool str_is_empty(const char* s)
{
  return str_size(s) == 0;
}

bool str_is_whitespace(const char* s)
{
  size_t i, n, size;
  size = str_size(s);
  n = 0;
  for (i = 0; i < size; i++) {
    if (isspace(s[i]))
      n++;
  }
  return n == size;
}

size_t str_size(const char* s)
{
  if (s)
    return strlen(s);
  else
    return 0;
}

char *str_copy(const char* s)
{
  char *d = NULL;
  size_t i, size;

  size = str_size(s);
  if (size > 0) {
    d = malloc(sizeof(char) * (size + 1));
    for (i = 0; i < size; i++) {
        d[i] = s[i];
    }
    d[size] = '\0';
  }
  return d;
}

bool str_equal(const char* a, const char* b)
{
  size_t i, na, nb;
  na = str_size(a);
  nb = str_size(b);

  if (na == nb) {
    for (i = 0; i < na; i++) {
      if (a[i] != b[i])
        return false;
    }
  }
  else {
    return false;
  }

  return true;
}
