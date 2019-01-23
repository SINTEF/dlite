#include <string.h>

#include "dlite.h"
#include "philibtable.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s


int main()
{
  size_t nelements=3, nphases=2, nvars=1, nbounds=2 , nconds=2 , ncalc=2, npoints=2;
  char *elements[] = {"Al", "Mg", "Si"};
  char *phases[] = {"FCC_A1", "MG2SI"};
  size_t i, j;
  double tmp, atvol0;

  size_t dims[] = {nelements, nphases, nvars, nbounds , nconds , ncalc, npoints};
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

  p->phaseselementdep[0]=0;	// Matrix depends on main element Al
  p->phaseselementdep[1]=1;	// MG2SI depends on element Mg

  p->varnames[0]=strdup("T");
//  p->varranges[0][0]=500.;	// in K
//  p->varranges[0][1]=800.;	// in K

  p->condnames[0]=strdup("X0(Mg)");
  p->condvalues[0]=0.1;		// in at%
  p->condnames[1]=strdup("X0(Si)");
  p->condvalues[1]=0.1;		// in at%

  p->calcnames[0]=strdup("fv(MG2SI)");
  p->calcnames[1]=strdup("X(MG2SI,Si)");

//  p->calcvalues[0,0]=0.2;		// "fv(MG2SI)" at 500 K
//  p->calcvalues[0,1]=0.1;		// "fv(MG2SI)" at 800 K
  
//  p->calcvalues[1,0]=0.33;		// "X(MG2SI,Si)" at 500 K
//  p->calcvalues[1,1]=0.38;		// "X(MG2SI,Si)" at 800 K


  /* Save instance */
  s = dlite_storage_open("json", "example-AlMgSi.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)p);
  dlite_storage_close(s);


  /* Free instance and its entity */
  dlite_instance_decref((DLiteInstance *)p);
  dlite_meta_decref(table);

  return 0;
}
