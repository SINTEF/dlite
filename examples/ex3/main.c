#include <string.h>

#include "dlite.h"
#include "philibtable.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s

char* fvname(char* phase){
   char* head = "fv(";
   char* tail = ")";
   char* string = (char*)malloc(1+strlen(head)+strlen(phase)+strlen(tail));

   if (!string) return NULL;
   strcpy(string, head);
   strcat(string, phase);
   strcat(string, tail);

   //printf("%s\n",string);

   return string;
}

char* xname(char* phase,char* element){
   char* head = "X(";
   char* middle = ",";
   char* tail = ")";
   char* string = (char*)malloc(1+strlen(head)+strlen(phase)+strlen(middle)+strlen(element)+strlen(tail));

   if (!string) return NULL;
   strcpy(string, head);
   strcat(string, phase);
   strcat(string, middle);
   strcat(string, element);
   strcat(string, tail);

   //printf("%s\n",string);

   return string;
}

int searchstring(char** arraystr,char* key, int size){

  int i, iloc=-1;

   for (i=0; i<size; i++) {
     if(strcmp(arraystr[i], key) == 0){
       iloc=i;
       break;
     }
   }

   return iloc;
}

double bounding(double value, double vmin,double vmax){
   double val=value;
   if(val>vmax) val=vmax;
   if(val<vmin) val=vmin;
   return val;
}

int checkInBounds(double value, double vmin,double vmax){
   int test=0;
   if(value>vmax) test=1;
   if(value<vmin) test=-1;
   return test;
}

int main()
{

  size_t nelements=3, nphases=2, nvars=1, nbounds=2 , nconds=2 , ncalc=2, npoints=2;
  char *elements[] = {"Al", "Mg", "Si"};
  char *phases[] = {"FCC_A1", "MG2SI"};
  size_t i, j, ielt;
  size_t dims[] =
    {nelements, nphases, nvars, nbounds , nconds , ncalc, npoints};


  char *path = "PhilibTable.json";
  DLiteStorage *s;
  DLiteMeta *table;
  PhilibTable *p;

  char* str1, *xstr;
  int iloc;
  double list_fv[nphases];
  double list_comp[nphases][nelements];
  double value;
  double sum;

  /* Load PhilibTable entity */
  s = dlite_storage_open("json", path, "mode=r");
  char *uri = "http://onto-ns.com/meta/philib/0.1/PhilibTable";
  table = (DLiteMeta *)dlite_meta_load(s, uri);
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
  p->varranges[0*ncalc+0]=500.;	// in K
  p->varranges[0*ncalc+1]=800.;	// in K

  p->condnames[0]=strdup("X0(Mg)");
  p->condvalues[0]=0.1;		// in at%
  p->condnames[1]=strdup("X0(Si)");
  p->condvalues[1]=0.1;		// in at%

  p->calcnames[0]=strdup("fv(MG2SI)");
  p->calcnames[1]=strdup("X(MG2SI,Si)");

  p->calcvalues[0*ncalc+0]=0.2;		// "fv(MG2SI)" at 500 K
  p->calcvalues[0*ncalc+1]=0.1;		// "fv(MG2SI)" at 800 K

  p->calcvalues[1*ncalc+0]=0.33;	// "X(MG2SI,Si)" at 500 K
  p->calcvalues[1*ncalc+1]=0.38;	// "X(MG2SI,Si)" at 800 K


  xstr=xname(p->phases[1], p->elements[2]);

  str1=fvname(p->phases[1]);
  iloc=searchstring(p->calcnames, str1, ncalc);
  free(str1);
  //iloc=searchstring(p->phases,p->phases[1],nphases);
  printf("position: %d\n",iloc);

  /* Save instance */
  s = dlite_storage_open("json", "example-AlMgSi.json", "mode=w;single=1");
  dlite_instance_save(s, (DLiteInstance *)p);
  dlite_storage_close(s);

  /* What we expect from philib */
  /* Get the material state  	*/
  /* output:			*/
  /*	    - list_fv[nphases]			*/
  /*	    - list_comp[nphases,nelements]	*/

  // List of volume fractions
  /* Make the list of all calculated volume fraction
   list of values of all the fv for all phases (except first phase that is the dependent one) ordered*/
  sum=0.0;
  for (i=1; i<nphases; i++){
	char *s=fvname(p->phases[i]);
  	// find the corresponding calcnames if not raise error
	iloc=searchstring(p->calcnames,s,ncalc);
        free(s);
	if(iloc!=-1){
           value=bounding(p->calcvalues[iloc*ncalc+0],0.0,1.0);	// add a filter to ensure realistic value
        }
	else{
	   printf("Error: volume fraction for phase %s not found\n",p->phases[i]); // raise an error
           value=0.0;
	}
	list_fv[i]=value; // add the value to the list
	sum += value;
  }

  // compute fv for the first phase (dependent)
  list_fv[0] = 1. - sum;
  // check if the dependent value is in bounds
  if(checkInBounds(list_fv[0],0.0,1.0)!=0) printf("Error: volume fraction for phase 0 out of bounds\n");

  printf("------ list_fv ------\n");
  for(i=0; i<nphases; i++){
    char *s = fvname(p->phases[i]);
    printf("%s =%f\n", s, list_fv[i]);
    free(s);
  }


  // list of composition
  //go through all the phases except first
  for(i=1; i<nphases; i++){
	sum=0.0;
	for(ielt=0; ielt<nelements; ielt++){
	   char *s=xname(p->phases[i],p->elements[ielt]);
	// find the corresponding calcnames else iloc=-1
	   iloc=searchstring(p->calcnames,s,ncalc);
           free(s);
	   if(iloc>0){
		value=bounding(p->calcvalues[iloc*ncalc+0],0.0,1.0);
		list_comp[i][ielt]=value;
	   }
	   else{
		list_comp[i][ielt]=0.0;
	   }
	   sum +=list_comp[i][ielt];
	}
	// compute the dependent element
	list_comp[i][p->phaseselementdep[i]]=1. - sum;
   }

   // deal with the matrix composition
   //for( int ielt=0;ielt<nelements;ielt++){
   //	list_comp[0][ielt] = X0 (elt) - sum ( list_fv[phase] * list_comp(phase,elt) )

  printf("------ list_comp ------\n");
  for(i=1; i<nphases; i++){
    for(ielt=0; ielt<nelements; ielt++){
      char *s = xname(p->phases[i],p->elements[ielt]);
      printf("%s =%f\n", s, list_comp[i][ielt]);
      free(s);
    }
  }
   // final check that all these values are not outside bounds



  /* Free instance and its entity */
  free(xstr);
  dlite_instance_decref((DLiteInstance *)p);
  dlite_meta_decref(table);

  return 0;
}
