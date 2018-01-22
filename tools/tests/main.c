#include <string.h>

#include "dlite.h"
#include "chemistry.h"


int main()
{
  int nelements=4, nphases=3;
  chemistry_s *chem = chemistry_create_with_id(nelements, nphases,
					       "example-6xxx");
  chemistry_properties_s *p = chemistry_props(chem);
  DLiteStorage *s = dlite_storage_open("hdf5", "example-6xxx.h5", "w");

  char *elements[] = {"Al", "Mg", "Si", "Fe"};
  char *phases[] = {"FCC_A1", "MG2SI", "ALFESI_ALPHA"};
  int i, j;
  double tmp, atvol0;

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

  chemistry_save(chem, s);

  free(p->alloy);
  for (i=0; i<nelements; i++) free(p->elements[i]);
  for (j=0; j<nphases; j++) free(p->phases[j]);
  chemistry_free(chem);
  dlite_storage_close(s);

  /********************************************************/

  s = dlite_storage_open("hdf5", "example-6xxx.h5", "r");
  chem = chemistry_load(s, "example-6xxx");
  dlite_storage_close(s);


  s = dlite_storage_open("hdf5", "example2-6xxx.h5", "w");
  chemistry_save(chem, s);
  dlite_storage_close(s);

  chemistry_free(chem);


  return 0;
}
