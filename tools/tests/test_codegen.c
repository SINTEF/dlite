#include <string.h>

#include "dlite.h"
#include "chemistry.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s


int main()
{
  size_t nelements=4, nphases=3;
  char *elements[] = {"Al", "Mg", "Si", "Fe"};
  char *phases[] = {"FCC_A1", "MG2SI", "ALFESI_ALPHA"};
  size_t i, j;
  double tmp, atvol0;

  size_t dims[] = {nelements, nphases};
  char *path = STRINGIFY(DLITE_ROOT) "/tools/tests/Chemistry-0.1.json";
  DLiteStorage *s;
  DLiteEntity *chem;
  Chemistry *p;

  /* Load Chemistry entity */
  s = dlite_storage_open("json", path, "mode=r");
  chem = dlite_entity_load(s, "http://www.sintef.no/calm/0.1/Chemistry");
  dlite_storage_close(s);

  /* Create instance */
  p = (Chemistry *)dlite_instance_create(chem, dims, "example-6xxx");

  p->alloy = strdup("Sample alloy...");

  for (i=0; i<nelements; i++)
    p->elements[i] = strdup(elements[i]);

  for (j=0; j<nphases; j++)
    p->phases[j] = strdup(phases[j]);

  p->X0[0] = 1.0;
  p->X0[1] = 0.5e-2;
  p->X0[2] = 0.5e-2;
  p->X0[3] = 0.03e-2;
  for (i=1; i<nelements; i++)
    p->X0[0] -= p->X0[i];

  p->volfrac[0] = 0.98;
  p->volfrac[1] = 0.01;
  p->volfrac[2] = 0.01;

  p->rpart[0] = 0.0;
  p->rpart[1] = 1e-6;
  p->rpart[2] = 10e-6;

  p->atvol[0] = 16e-30;
  p->atvol[1] = 24e-30;
  p->atvol[2] = 20e-30;

  p->Xp[nelements + 0] = 0.0;
  p->Xp[nelements + 1] = 2.0 / 3.0;
  p->Xp[nelements + 2] = 1.0 / 3.0;
  p->Xp[nelements + 3] = 0.0;

  p->Xp[2*nelements + 0] = 0.7;
  p->Xp[2*nelements + 1] = 0.0;
  p->Xp[2*nelements + 2] = 0.1;
  p->Xp[2*nelements + 3] = 0.2;

  tmp = 0.0;
  for (j=1; j<nphases; j++) tmp += p->volfrac[j] / p->atvol[j];
  atvol0 = 1.0 / tmp;

  for (i=0; i<nelements; i++) p->Xp[i] = p->X0[i];
  for (j=1; j<nphases; j++)
    for (i=0; i<nelements; i++)
      p->Xp[i] -= atvol0/p->atvol[j] * p->volfrac[j] * p->Xp[j*nelements + i];


  /* Save instance */
  s = dlite_storage_open("json", "example-6xxx.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)p);
  dlite_storage_close(s);

  /* Free instance and its entity */
  dlite_instance_decref((DLiteInstance *)p);
  dlite_entity_decref(chem);

  return 0;
}
