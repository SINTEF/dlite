#include <string.h>

#include "dlite.h"
#include "chemistry.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s


/*
  This example will create a DLite entity based on the metadata description in
  Chemistry-0.1.json. The empty instance will then be populated with values,
  and the instance is then stored to disk as a json-file
*/ 
int main()
{
  /*
   This example creates an alloy with four elements (Aluminium, Manganese,
   Silicon and Iron) with three different phases. The number of elements and
   number of phases determines the size of the dimension in the DLite instance,
   and is required to allocate the correct amount of memory in the constructor
   of the instance
  */
  size_t nelements=4, nphases=3;
  char *elements[] = {"Al", "Mg", "Si", "Fe"};
  char *phases[] = {"FCC_A1", "MG2SI", "ALFESI_ALPHA"};
  size_t i, j;
  double tmp, atvol0;

  /*
   The array of dimensions is required to construct the instance
  */
  size_t dims[] = {nelements, nphases};
  /*
   We need the path to the entity definition to be able to create the entity
   instance
   */
  char *path = STRINGIFY(DLITE_ROOT)
    "/share/dlite/examples/ex1/Chemistry-0.1.json";
  DLiteStorage *s;
  DLiteMeta *chem;
  Chemistry *p;

  /* Load Chemistry entity */
  s = dlite_storage_open("json", path, "mode=r");
  chem = (DLiteMeta *)
    dlite_meta_load(s, "http://sintef.no/calm/0.1/Chemistry");
  dlite_storage_close(s);

  /* Create instance */
  p = (Chemistry *)dlite_instance_create(chem, dims, "example-6xxx");

  /* Set alloy description */
  p->alloy = strdup("Sample alloy...");

  /* Copy element names into array */
  for (i=0; i<nelements; i++)
    p->elements[i] = strdup(elements[i]);

  /* Copy phase names into array */
  for (j=0; j<nphases; j++)
    p->phases[j] = strdup(phases[j]);

  /* Set nominal composition, and make sure the sum is 1.0 */
  p->X0[0] = 1.0;
  p->X0[1] = 0.5e-2;
  p->X0[2] = 0.5e-2;
  p->X0[3] = 0.03e-2;
  for (i=1; i<nelements; i++)
    p->X0[0] -= p->X0[i];

  /* Set volume fraction of each phase, excluding matrix */
  p->volfrac[0] = 0.98;
  p->volfrac[1] = 0.01;
  p->volfrac[2] = 0.01;

  /* Set average particle radius of each phase, excluding matrix */
  p->rpart[0] = 0.0;
  p->rpart[1] = 1e-6;
  p->rpart[2] = 10e-6;

  /* Set average volume per atom for each phase */
  p->atvol[0] = 16e-30;
  p->atvol[1] = 24e-30;
  p->atvol[2] = 20e-30;

  /* Set Average composition for phase 2 */
  p->Xp[nelements + 0] = 0.0;
  p->Xp[nelements + 1] = 2.0 / 3.0;
  p->Xp[nelements + 2] = 1.0 / 3.0;
  p->Xp[nelements + 3] = 0.0;

  /* Set average composition for phase 3 */
  p->Xp[2*nelements + 0] = 0.7;
  p->Xp[2*nelements + 1] = 0.0;
  p->Xp[2*nelements + 2] = 0.1;
  p->Xp[2*nelements + 3] = 0.2;

  /* Calculate average composition for phase 1 and set*/
  tmp = 0.0;
  for (j=1; j<nphases; j++) tmp += p->volfrac[j] / p->atvol[j];
  atvol0 = 1.0 / tmp;

  for (i=0; i<nelements; i++) p->Xp[i] = p->X0[i];
  for (j=1; j<nphases; j++)
    for (i=0; i<nelements; i++)
      p->Xp[i] -= atvol0/p->atvol[j] * p->volfrac[j] * p->Xp[j*nelements + i];


  /*
   Save instance as json to example-6xxx.json
   First a storage pointer is created, next the instance is saved, and finally
   the pointer is closed
  */
  s = dlite_storage_open("json", "example-6xxx.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)p);
  dlite_storage_close(s);

  /* Free instance and its entity */
  dlite_instance_decref((DLiteInstance *)p);
  dlite_meta_decref(chem);

  return 0;
}
