/* dlite-plugins-python.c -- a DLite plugin for plugins written in python */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Python.h"

/* #include "config.h" */

#include "boolean.h"

#include "dlite.h"
#include "dlite-datamodel.h"


static int python_initialised=0;


/* Initialises python */
void init_python(void)
{
  wchar_t *progname;
  if (python_initialised) return;
  if (!(progname = Py_DecodeLocale("dlite", NULL))) {
    dlite_err(1, "allocation failure");
    return;
  }
  Py_SetProgramName(progname);
  PyMem_RawFree(progname);
  Py_Initialize();
  python_initialised = 1;
}

/* Finalizes python */
int finilize_python(void)
{
  int status;
  if (!python_initialised) return -1;
  status = Py_FinalizeEx();
  python_initialised = 0;
  return status;
}
