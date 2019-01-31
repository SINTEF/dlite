/************************************************************************/
/*                                                                      */
/*    This sample program shows how to retrieve data from a Thermo-     */
/*    Calc data file, then define a set of conditions for a single      */
/*    equilibrium calculation, get the equilibrium phases and their     */
/*    amounts and compositions. The method of calculating the liquidus  */
/*    and solidus temperature is also demonstrated.                     */
/*                                                                      */
/************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "tqroot.h"

#include "dlite.h"
#include "philibtable.h"


void writepx(TC_INT iph,TC_INT* iwsg,TC_INT* iwse)
{
  TC_STRING phname,sname;
  TC_INT i;
  TC_FLOAT val,valnp,valw;

  phname=malloc(TC_STRLEN_PHASES);
  sname=malloc(TC_STRLEN_MAX);

  fprintf(stdout,"phase name    composition, Wt percent Cu     amount\n");
  for (i=0; i<iph; i++) {
    tq_get1("DG", i+1, -1, &val, iwsg,iwse);
    if(val == 0.0) {
      tq_gpn(i+1, phname, TC_STRLEN_PHASES, iwsg,iwse);
      tq_get1("np", i+1, -1, &valnp, iwsg,iwse);
      tq_get1("w%", i+1,  2, &valw,  iwsg,iwse);
      if (valnp < TC_EPS) { valnp=0.0;}
      fprintf(stdout,"%8s          %16g %16g\n",phname,valw,valnp);
    }
  }
  fprintf(stdout,"\n");

  free(phname);
  free(sname);
}

void get_fv(TC_INT iph, TC_INT icomp, TC_INT* iwsg, TC_INT* iwse, double fv[] , double comp[])
{
	TC_STRING phname, sname;
	TC_INT i, j;
	TC_FLOAT val, valnp;
	TC_FLOAT* valw;
	TC_INT icont;

	valw = malloc(sizeof(TC_FLOAT)*icomp);

	phname = malloc(TC_STRLEN_PHASES);
	sname = malloc(TC_STRLEN_MAX);

	fprintf(stdout, "phase name    composition, Wt percent Cu     amount\n");
	for (i = 0; i < iph; i++) {
		tq_get1("DG", i + 1, -1, &val, iwsg, iwse);
		if (val == 0.0) {
			tq_gpn(i + 1, phname, TC_STRLEN_PHASES, iwsg, iwse);
			tq_get1("np", i + 1, -1, &valnp, iwsg, iwse);
			fv[i] = (double) valnp;
			
			for (j = 0; j < icomp; j++)
			{
				tq_get1("w%", i + 1, j + 1, &valw[j], iwsg, iwse);
				comp[i*icomp+j] = (double) valw[j];
			}

			if (valnp < TC_EPS) { valnp = 0.0; }
			fprintf(stdout, "%8s          %16g %16g %16g\n", phname, valw[0], valw[1], valnp);
		}
		else
		{
			tq_gpn(i + 1, phname, TC_STRLEN_PHASES, iwsg, iwse); 
			fprintf(stdout, "%8s   is not there.\n", phname);
			fv[i] = 0.0;
			for (j = 0; j < icomp; j++)
			{
				comp[i*icomp+j] = 0.0;
			}
		}
	}
	fprintf(stdout, "\n");

	free(phname);
	free(sname);
	free(valw);
}



main(int argc, char *argv)
{
	size_t dims[7];

	char *path = "PhilibTable.json";
	DLiteStorage *s;
	DLiteMeta *table;
	PhilibTable *p;

	/* ----------------------------------------------------------*/
	/*                                                           */
	/*            Dlite entity creation                          */
	/*                                                           */
	/* ----------------------------------------------------------*/

	/* Load PhilibTable entity */
	s = dlite_storage_open("json", path, "mode=r");
	char *uri = "http://meta.sintef.no/philib/0.1/PhilibTable";
	table = (DLiteMeta *)dlite_meta_load(s, uri);
	size_t ndims = table->ndimensions;
	dlite_storage_close(s);

	/* ----------------------------------------------------------*/
    /* ----------------------------------------------------------*/

  TC_INT *iwsg,*iwse;
  tc_components_strings *components;
  TC_INT i,j,ncomp,iph,iliq;
  TC_INT icont,iconn,iconp,iconw;
  TC_BOOL pstat;
  TC_STRING phname,sname;
  TC_FLOAT an,val,valt;
  TC_INT ipp,ipm,imc;
  TC_FLOAT vm,vp,surface_energy;
  TC_INT ierr;
  char log_file_directory[FILENAME_MAX];
  char tc_installation_directory[FILENAME_MAX];
  
  ierr = 0;
  iwsg=malloc(sizeof(TC_INT)*TC_NWSG);
  iwse=malloc(sizeof(TC_INT)*TC_NWSE);
  phname=malloc(TC_STRLEN_PHASES);
  sname=malloc(TC_STRLEN_MAX);
  components=malloc(TC_MAX_NR_OF_ELEMENTS*sizeof(tc_components_strings));

  memset(iwsg,0,sizeof(TC_INT)*TC_NWSG);
  memset(iwse,0,sizeof(TC_INT)*TC_NWSE);
  memset(log_file_directory,0,sizeof(log_file_directory));
  memset(tc_installation_directory,0,sizeof(tc_installation_directory));
    
  /* initiate the workspace */
  tq_ini3(tc_installation_directory,log_file_directory,TC_NWSG,TC_NWSE,iwsg,iwse);

  /* read the thermodynamic data file which was created by using */
  /* the GES module inside the Thermo-Calc software package */
  tq_rfil("TQEX01",iwsg,iwse);
  if(tq_sg1err(&ierr))return;

  /* get component names in the system */
  tq_gcom(&ncomp,components,iwsg,iwse);
  if(tq_sg1err(&ierr))return;

  fprintf(stdout,"This system has the following components:\n");
  for (i=0; i < ncomp; i++) {
    fprintf(stdout,"%d %s\n",i+1,components[i].component);
  }
  fprintf(stdout,"\n");

  /* get number of phases in the system */
  tq_gnp(&iph, iwsg,iwse);
  if(tq_sg1err(&ierr))return;
  fprintf(stdout,"This system has %d phases:\n",iph);
  if(tq_sg1err(&ierr))return;
  /* get names and status of the phases in the system */
  for (i=0; i<iph; i++) {
    tq_gpn(i+1, phname, TC_STRLEN_PHASES, iwsg,iwse);
    if(tq_sg1err(&ierr))return;
    pstat=tq_gsp(i+1, sname, TC_STRLEN_MAX, &an, iwsg,iwse);
    fprintf(stdout,"%s %s %g\n",phname,sname, an);
  }
  fprintf(stdout,"\n");

  /* ----------------------------------------------------------*/
  /*                                                           */
  /*            Dlite instance creation and filling            */
  /*                                                           */
  /* ----------------------------------------------------------*/

  dims[0] = ncomp;
  dims[1] = iph;
  size_t nvars = 2;
  dims[2] = nvars;		// nvars, the temperature and composition will vary
  dims[3] = 2 ; //nbounds
  dims[4] = 1; // nconds
  size_t ncalc = 2;
  size_t npoints = 12; // this variable is related to ticks so strange to define it before
  dims[5] = ncalc;
  dims[6] = npoints;

  //TQ_main();

  /* Create instance */
  p = (PhilibTable *)dlite_instance_create(table, dims, "example-AlMgSi");

  /* transfer the name of the elements (already read) */
  for (i = 0; i < ncomp; i++)
	  p->elements[i] = strdup(components[i].component);
  
  /* transfer the name of the phases */
  for (i = 0; i < iph; i++) {
	  tq_gpn(i + 1, phname, TC_STRLEN_PHASES, iwsg, iwse);
	  if (tq_sg1err(&ierr))return;
	  p->phases[i]=strdup(phname);
  }

  p->varnames[0] = strdup("T");
  p->varranges[0 * nvars + 0] = 530.;	// in deg C
  p->varranges[0 * nvars + 1] = 630.;	// in deg C

  p->varnames[1] = strdup("W%(Cu)");
  p->varranges[1 * nvars + 0] = 6.0;	// in W%
  p->varranges[1 * nvars + 1] = 8.0;	// in W%

  /* define the discretization for all variables */
  p->ticks[0] = 4;	// for the temperature
  p->ticks[1] = 3;	// for the concentration


  /* Save instance */
  s = dlite_storage_open("json", "example-AlMgSi.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)p);
  dlite_storage_close(s);

  /* ----------------------------------------------------------*/
  /* ----------------------------------------------------------*/

  /* set the units of some properties. */
  tq_ssu((TC_STRING)"ENERGY",(TC_STRING)"CAL",iwsg,iwse);
  if(tq_sg1err(&ierr))return;
  tq_ssu((TC_STRING)"T",(TC_STRING)"C",iwsg,iwse);
  if(tq_sg1err(&ierr))return;

  /* set the condition for a sigle equilibrium calculation */
  tq_setc("N", -1, -1, 1.00, &iconn, iwsg,iwse);
  tq_setc("P",-1,-1,101325.0,&iconp,iwsg,iwse);

  /* ----------------------------------------------------------*/
  /*                                                           */
  /*            Dlite storing the calculated values            */
  /*                                                           */
  /* ----------------------------------------------------------*/
	/* test of new subroutine */
  double* fv; double* comp;
  fv = malloc(sizeof(double)*iph);
  comp = malloc(sizeof(double)*iph*ncomp);

  double temp;
  double conc;

  p->calcnames[0] = strdup("fv(LIQUID)");
  p->calcnames[1] = strdup("X(LIQUID,Cu)");

  for (size_t iTemp = 0; iTemp < p->ticks[0]; iTemp++) {
	  for (size_t iConc = 0; iConc < p->ticks[1]; iConc++) {
		  temp = (p->varranges[0 * nvars + 1] - p->varranges[0 * nvars + 0])*iTemp/ (p->ticks[0]-1) + p->varranges[0 * nvars + 0];
		  conc = (p->varranges[1 * nvars + 1] - p->varranges[1 * nvars + 0])*iConc / (p->ticks[1] - 1) + p->varranges[1 * nvars + 0];

		  size_t ipos = (iTemp + p->ticks[0] * iConc);
		  p->points[ipos*nvars + 0] = temp;
		  p->points[ipos*nvars + 1] = conc;

		  /* set the conditions */
		  tq_setc("T", -1, -1, temp, &icont, iwsg, iwse);
		  tq_setc("W%", -1, 2, conc, &iconw, iwsg, iwse);
		  /* calculate equilibrium */
		  tq_ce(" ", 0, 0, 0.0, iwsg, iwse);
		  if (tq_sg1err(&ierr))return;

		  get_fv(iph, ncomp, iwsg, iwse, fv, comp);
		  p->calcvalues[ipos*nvars + 0 ] = fv[16];		// "fv(LIQUID)" at 630 K
		  p->calcvalues[ipos*nvars + 1 ] = comp[16 * ncomp + 1];	// "X(LIQUID,Cu)" at 630 K

	  }
  }







  /* Save instance */
  s = dlite_storage_open("json", "example-AlMgSi.json", "mode=w");
  dlite_instance_save(s, (DLiteInstance *)p);
  dlite_storage_close(s);

  /* ----------------------------------------------------------*/
  /* ----------------------------------------------------------*/

  free(components);
  free(phname);
  free(sname);
  free(iwsg);
  free(iwse);
  free(fv);
  free(comp);
}
