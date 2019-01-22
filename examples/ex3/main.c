#include <string.h>

#include "dlite.h"
#include "philibtable.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s


int main()
{
  size_t nelements=4, nphases=3;
  char *elements[] = {"Al", "Mg", "Si"};
  char *phases[] = {"FCC_A1", "MG2SI"};
  size_t i, j;
  double tmp, atvol0;

  size_t dims[] = {nelements, nphases};
  char *path = "PhilibTable.json";
  DLiteStorage *s;
  DLiteMeta *table;
  PhilibTable *p;

  // new variables
  char *id ="mydata";
  PhilibTable *pload;

  /* Load PhilibTable entity */
  s = dlite_storage_open("json", path, "mode=r");
  table = (DLiteMeta *)
    dlite_meta_load(s, "http://meta.sintef.no/philib/0.1/PhilibTable");
  dlite_storage_close(s);

  /* Create instance */
  p = (PhilibTable *)dlite_instance_create(table, dims, "example-AlMgSi");

  for (i=0; i<nelements; i++)
    p->elements[i] = strdup(elements[i]);

  for (j=0; j<nphases; j++)
    p->phases[j] = strdup(phases[j]);

  /* Save instance */
  s = dlite_storage_open("json", "example-AlMgSi.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)p);
  dlite_storage_close(s);


  /* Free instance and its entity */
  dlite_instance_decref((DLiteInstance *)p);
  dlite_meta_decref(table);

  return 0;
}
