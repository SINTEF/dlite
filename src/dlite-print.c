#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "dlite.h"
#include "dlite-macros.h"


/* Expands to `a - b` if `a > b` else to `0`. */
#define PDIFF(a, b) (((size_t)(a) > (size_t)(b)) ? (a) - (b) : 0)

/* These macros could have been simplified with __VA_ARGS__, but not
   all compilers support that... */
#define PRINT(fmt)                                      \
  do {                                                  \
    int m = snprintf(dest+n, PDIFF(size, n), fmt);      \
    if (m < 0) goto fail;                               \
    n += m;                                             \
  } while (0)
#define PRINT1(fmt, a1)                                 \
  do {                                                  \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1);  \
    if (m < 0) goto fail;                               \
    n += m;                                             \
  } while (0)
#define PRINT2(fmt, a1, a2)                                     \
  do {                                                          \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1, a2);      \
    if (m < 0) goto fail;                                       \
    n += m;                                                     \
  } while (0)
#define PRINT3(fmt, a1, a2, a3)                                 \
  do {                                                          \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1, a2, a3);  \
    if (m < 0) goto fail;                                       \
    n += m;                                                     \
  } while (0)
#define PRINT4(fmt, a1, a2, a3, a4)                                 \
  do {                                                              \
    int m = snprintf(dest+n, PDIFF(size, n), fmt, a1, a2, a3, a4);  \
    if (m < 0) goto fail;                                           \
    n += m;                                                         \
  } while (0)



/*
  Prints instance `inst` to `dest`, formatted as JSON.
*/
int dlite_print(char *dest, size_t size, DLiteInstance *inst,
                int indent, DLitePrintFlag flags)
{
  DLiteTypeFlag f = (DLiteTypeFlag)flags;
  int n=0, ok=0, m;
  size_t i;
  char *in = malloc(indent + 1);
  memset(in, ' ', indent);
  in[indent] = '\0';

  PRINT1("%s{\n", in);
  if (flags & dlitePrintUUID)
    PRINT2("%s  \"uuid\": \"%s\",\n", in, inst->uuid);
  if (inst->uri)
    PRINT2("%s  \"uri\": \"%s\",\n", in, inst->uri);
  PRINT2("%s  \"meta\": \"%s\",\n", in, inst->meta->uri);
  PRINT1("%s  \"dimensions\": {\n", in);
  for (i=0; i < inst->meta->_ndimensions; i++) {
    char *name = inst->meta->_dimensions[i].name;
    int val = DLITE_DIM(inst, i);
    char *c = (i < inst->meta->_ndimensions - 1) ? "," : "";
    PRINT4("%s    \"%s\": %d%s\n", in, name, val, c);
  }
  PRINT1("%s  },\n", in);

  PRINT1("%s  \"properties\": {\n", in);
  for (i=0; i < inst->meta->_nproperties; i++) {
    char *c = (i < inst->meta->_nproperties - 1) ? "," : "";
    DLiteProperty *p = inst->meta->_properties + i;
    void *ptr = dlite_instance_get_property_by_index(inst, i);
    size_t *dims = DLITE_PROP_DIMS(inst, i);
    PRINT2("%s    \"%s\": ", in, p->name);
    m = dlite_property_print(dest+n, PDIFF(size, n), ptr, p, dims, 0, -2, f);
    if (m < 0) return -1;
    n += m;
    PRINT1("%s\n", c);
  }
  PRINT1("%s  }\n", in);
  PRINT1("%s}", in);

  ok = 1;
 fail:
  free(in);
  return (ok) ? n : -1;
}
