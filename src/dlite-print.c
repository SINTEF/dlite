#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "dlite.h"
#include "dlite-macros.h"


/* Expands to `a - b` if `a > b` else to `0`. */
#define PDIFF(a, b) (((size_t)(a) > (size_t)(b)) ? (a) - (b) : 0)


/*
  Prints instance `inst` to buffer, formatted as JSON.
*/
int dlite_print(char *dest, size_t size, DLiteInstance *inst,
                int indent, DLitePrintFlag flags)
{
  DLiteTypeFlag f = (DLiteTypeFlag)flags;
  int n=0, N=size, m;
  size_t i;
  char *in = malloc(indent + 1);
  UNUSED(flags);
  memset(in, ' ', indent);
  in[indent] = '\0';

  m = snprintf(dest+n, PDIFF(N, n), "%s{\n", in);
  if (m < 0) return -1;
  n += m;
  if (flags & dlitePrintUUID) {
    m = snprintf(dest+n, PDIFF(N, n), "%s  \"uuid\": \"%s\",\n",
                 in, inst->uuid);
    if (m < 0) return -1;
    n += m;
  }
  if (inst->uri) {
    m = snprintf(dest+n, PDIFF(N, n), "%s  \"uri\": \"%s\",\n",
                 in, inst->uri);
    if (m < 0) return -1;
    n += m;
  }
  m = snprintf(dest+n, PDIFF(N, n), "%s  \"meta\": \"%s\",\n",
               in, inst->meta->uri);
  if (m < 0) return -1;
  n += m;

  m = snprintf(dest+n, PDIFF(N, n), "%s  \"dimensions\": {\n", in);
  if (m < 0) return -1;
  n += m;
  for (i=0; i < inst->meta->_ndimensions; i++) {
    char *name = inst->meta->_dimensions[i].name;
    int val = DLITE_DIM(inst, i);
    char *c = (i < inst->meta->_ndimensions - 1) ? "," : "";
    m = snprintf(dest+n, PDIFF(N, n), "%s    \"%s\": %d%s\n", in, name, val, c);
    if (m < 0) return -1;
    n += m;
  }
  m = snprintf(dest+n, PDIFF(N, n), "%s  },\n", in);
  if (m < 0) return -1;
  n += m;

  m = snprintf(dest+n, PDIFF(N, n), "%s  \"properties\": {\n", in);
  if (m < 0) return -1;
  n += m;
  for (i=0; i < inst->meta->_nproperties; i++) {
    char *c = (i < inst->meta->_nproperties - 1) ? "," : "";
    DLiteProperty *p = inst->meta->_properties + i;
    void *ptr = dlite_instance_get_property_by_index(inst, i);
    size_t *dims = DLITE_PROP_DIMS(inst, i);
    m = snprintf(dest+n, PDIFF(N, n), "%s    \"%s\": ", in, p->name);
    if (m < 0) return -1;
    n += m;
    m = dlite_property_print(dest+n, PDIFF(N, n), ptr, p, dims, 0, -2, f);
    if (m < 0) return -1;
    n += m;
    m = snprintf(dest+n, PDIFF(N, n), "%s\n", c);
    if (m < 0) return -1;
    n += m;
  }
  m = snprintf(dest+n, PDIFF(N, n), "%s  }\n", in);
  if (m < 0) return -1;
  n += m;

  m = snprintf(dest+n, PDIFF(N, n), "%s}", in);
  if (m < 0) return -1;
  n += m;
  free(in);
  return n;
}
