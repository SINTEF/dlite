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
#include "dlite-macros.h"
#include "dlite-datamodel.h"
#include "pyembed/dlite-pyembed.h"
#include "pyembed/dlite-python-storage.h"


/*
  Free's internal resources in `api`.
*/
static void freer(DLiteStoragePlugin *api)
{
  Py_XDECREF(api->data);
  free(api);
}


/*
  Returns API provided by storage plugin `name` implemented in Python.

  Default cost is 25.
*/
DSL_EXPORT const DLiteStoragePlugin *get_dlite_storage_api(int *iter)
{
  int n;
  DLiteStoragePlugin *api=NULL, *retval=NULL;
  PyObject *storages=NULL, *cls=NULL;
  const char *classname=NULL;

  if (!(storages = dlite_python_storage_load())) goto fail;
  assert(PyList_Check(storages));
  n = PyList_Size(storages);

  /* get class implementing the plugin API */
  if (*iter < 0 || *iter >= n)
    FAIL1("API iterator index is out of range: %d", *iter);
  cls = PyList_GetItem(storages, *iter);
  assert(cls);
  if (*iter < n - 1) (*iter)++;

  /* get classname for error messages */
  if (!(classname = dlite_pyembed_classname(cls)))
    dlite_warnx("cannot get class name for API %s", *((char **)api));

#if 0

  /* get attributes to fill into the api */
  if (!(name = PyObject_GetAttrString(cls, "name")))
    FAIL1("'%s' has no attribute: 'name'", classname);
  if (!PyUnicode_Check(name))
    FAIL1("attribute 'name' of '%s' is not a string", classname);

  if (!(out_uri = PyObject_GetAttrString(cls, "output_uri")))
    FAIL1("'%s' has no attribute: 'output_uri'", classname);
  if (!PyUnicode_Check(out_uri))
    FAIL1("attribute 'output_uri' of '%s' is not a string", classname);

  if (!(in_uris = PyObject_GetAttrString(cls, "input_uris")))
    FAIL1("'%s' has no attribute: 'input_uris'", classname);
  if (!PySequence_Check(in_uris))
    FAIL1("attribute 'input_uris' of '%s' is not a sequence", classname);

  if (!(input_uris = calloc(PySequence_Length(in_uris), sizeof(char *))))
    FAIL("allocation failure");
  for (i=0; i < PySequence_Length(in_uris); i++) {
    PyObject *in_uri = PySequence_GetItem(in_uris, i);
    if (!in_uri || !PyUnicode_Check(in_uri)) {
      Py_XDECREF(in_uri);
      FAIL2("item %d of attribute 'input_uris' of '%s' is not a string",
            i, classname);
    }
    input_uris[i] = PyUnicode_DATA(in_uri);
    Py_DECREF(in_uri);
  }

  if (!(map = PyObject_GetAttrString(cls, "map")))
    FAIL1("'%s' has no method: 'map'", classname);
  if (!PyCallable_Check(map))
    FAIL1("attribute 'map' of '%s' is not callable", classname);

  if ((pcost = PyObject_GetAttrString(cls, "cost")) && PyLong_Check(pcost))
    cost = PyLong_AsLong(pcost);
#endif

  if (!(api = calloc(1, sizeof(DLiteStoragePlugin))))
    FAIL("allocation failure");

  /*
  api->name = PyUnicode_DATA(name);
  api->output_uri = PyUnicode_DATA(out_uri);
  api->ninput = PySequence_Length(in_uris);
  api->input_uris = input_uris;
  api->mapper = mapper;
  api->freer = freer;
  api->cost = cost;
  api->data = (void *)cls;
  Py_INCREF(cls);
  */
  api->freer = freer;

  retval = api;
 fail:
  if (!retval && api) {
    free(api);
    api = NULL;
  }
  /*
  Py_XDECREF(name);
  Py_XDECREF(out_uri);
  Py_XDECREF(in_uris);
  Py_XDECREF(map);
  Py_XDECREF(pcost);
  */
  return api;
}
