
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

str_list_t *str_list()
{
  str_list_t *v;
  v = (str_list_t *) malloc(sizeof(str_list_t));
  v->capacity = 0;
  v->size = 0;
  v->data = NULL;
  return v;
}

str_list_t *str_list1(char *x, bool copy)
{
  str_list_t *v = str_list();
  str_list_reserve(v, 1);
  str_list_add(v, x, copy);
  return v;
}

str_list_t *str_list2(char *x, char *y, bool copy)
{
  str_list_t *v = str_list();
  str_list_reserve(v, 2);
  str_list_add(v, x, copy);
  str_list_add(v, y, copy);
  return v;
}

void str_list_add(str_list_t *v, char *value, bool copy)
{
  if (v->size >= v->capacity)
    str_list_reserve(v, 2 * v->size);
  v->data[v->size] = copy ? str_copy(value) : value;
  v->size++;
}

size_t str_list_size(str_list_t *v)
{
  return v ? v->size : 0;
}

void str_list_resize(str_list_t *v, size_t size)
{
  v->size = size;
  if (v->size >= v->capacity)
    str_list_reserve(v, 2 * v->size);
}

void str_list_reserve(str_list_t *v, size_t capacity)
{
  if (capacity == 0)
    capacity = 10;
  if (v->capacity < capacity) {
    if (v->data == NULL)
      v->data = (char **)malloc(capacity * sizeof(char*));
    else
      v->data = (char **)realloc(v->data, capacity * sizeof(char*));
    v->capacity = capacity;
  }
}

void str_list_free(str_list_t *v, bool free_items)
{
  size_t i;
  if (v) {
    if (v->data) {
      if (free_items) {
        for(i=0; i < v->size; i++)
          if (v->data[i])
            free(v->data[i]);
      }
      free(v->data);
    }
    free(v);
  }
}

void str_list_print(str_list_t *v, char* name)
{
  size_t i, last;
  if (v == NULL)
    printf("%s = NULL\n", name);
  else {
    printf("%s = [", name);
    last = v->size - 1;
    for(i=0; i < v->size; i++)
      if (i == last)
        printf("%s", v->data[i] ? v->data[i] : "NULL");
      else
        printf("%s, ", v->data[i] ? v->data[i] : "NULL");
    printf("]%c", '\n');
  }
}
