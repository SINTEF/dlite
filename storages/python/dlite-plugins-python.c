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
#include "dlite-storage-plugins.h"
#include "pyembed/dlite-pyembed.h"
#include "pyembed/dlite-python-storage.h"


typedef struct {
  DLiteStorage_HEAD
  PyObject *obj;      /* Python instance of storage class */
} DLiteJsonStorage;



DLiteStorage *
opener(const DLiteStoragePlugin *api, const char *uri, const char *options)
{
  DLiteJsonStorage *s=NULL;
  DLiteStorage *retval=NULL;
  PyObject *obj=NULL, *v=NULL, *writable=NULL;
  PyObject *cls = (PyObject *)api->data;
  const char *classname;

  if (!(classname = dlite_pyembed_classname(cls)))
    dlite_warnx("cannot get class name for storage plugin %s", *((char **)api));

  if (!(obj = PyObject_CallObject(cls, NULL)))
    FAIL1("error instantiating %s", classname);
  v = PyObject_CallMethod(obj, "open", "ss", uri, options);

  /* Check if the open() method has set attribute `writable` */
  if (PyObject_HasAttrString(obj, "writable"))
    writable = PyObject_GetAttrString(obj, "writable");




  if (!(s = calloc(1, sizeof(DLiteJsonStorage))))
    FAIL("Allocation failure");
  s->api = api;
  s->uri = strdup(uri);
  s->options = strdup(options);
  s->writable = (writable) ? PyObject_IsTrue(writable) : 1;
  s->obj = obj;

  //PyObject *cls = (PyObject *)api->data;
  //PyObject *pyuri = PyUnicode_FromString(uri);
  //PyObject *pyoptions = PyUnicode_FromString(options);
  //PyObject *open = PyObject_GetAttrString(cls, "open");
  //assert(open);
  //assert(PyCallable_Check(open));
  //
  //PyObject_CallFunctionObjArgs(open, uri, options, NULL);

  retval = (DLiteStorage *)s;
 fail:
  if (s && !retval) {
    free(s->uri);
    free(s->options);
    Py_DECREF(s->obj);
    free(s);
  }
  Py_XDECREF(v);
  Py_XDECREF(writable);
  return retval;
}


int closer(DLiteStorage *s)
{
  UNUSED(s);
  return 0;
}

DLiteInstance *instance_getter(const DLiteStorage *s, const char *uuid)
{
  UNUSED(s);
  UNUSED(uuid);
  return NULL;
}

int instance_setter(DLiteStorage *s, const DLiteInstance *inst)
{
  UNUSED(s);
  UNUSED(inst);
  return 0;
}



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
*/
DSL_EXPORT const DLiteStoragePlugin *get_dlite_storage_plugin_api(int *iter)
{
  int n;
  DLiteStoragePlugin *api=NULL, *retval=NULL;
  PyObject *storages=NULL, *cls=NULL, *name=NULL;
  PyObject *open=NULL, *close=NULL, *get_instance=NULL, *set_instance=NULL;
  const char *classname=NULL;

  printf("\n=== PythonStoragePlugin (iter=%d)\n", *iter);

  if (!(storages = dlite_python_storage_load())) goto fail;

  printf("\n=== storages: ");
  PyObject_Print(storages, stdout, 0);

  assert(PyList_Check(storages));
  n = PyList_Size(storages);

  printf("\n=== n=%d\n", n);

  /* get class implementing the plugin API */
  dlite_errclr();
  if (*iter < 0 || *iter >= n)
    FAIL1("API iterator index is out of range: %d", *iter);
  cls = PyList_GetItem(storages, *iter);
  assert(cls);
  if (*iter < n - 1) (*iter)++;

  /* get classname for error messages */
  if (!(classname = dlite_pyembed_classname(cls)))
    dlite_warnx("cannot get class name for storage plugin %s", *((char **)api));

  /* get attributes to fill into the api */
  if (!(name = PyObject_GetAttrString(cls, "name")))
    name = PyUnicode_FromString(classname);
  if (!PyUnicode_Check(name))
    FAIL1("attribute 'name' (or '__name__') of '%s' is not a string",
          (char *)PyUnicode_DATA(name));

  if (!(open = PyObject_GetAttrString(cls, "open")))
    FAIL1("'%s' has no method: 'open'", classname);
  if (!PyCallable_Check(open))
    FAIL1("attribute 'open' of '%s' is not callable", classname);

  if (!(close = PyObject_GetAttrString(cls, "close")))
    FAIL1("'%s' has no method: 'close'", classname);
  if (!PyCallable_Check(close))
    FAIL1("attribute 'close' of '%s' is not callable", classname);

  if (!(get_instance = PyObject_GetAttrString(cls, "get_instance")))
    FAIL1("'%s' has no method: 'get_instance'", classname);
  if (!PyCallable_Check(get_instance))
    FAIL1("attribute 'get_instance' of '%s' is not callable", classname);

  if ((set_instance = PyObject_GetAttrString(cls, "set_instance")) &&
      !PyCallable_Check(set_instance))
    FAIL1("attribute 'set_instance' of '%s' is not callable", classname);

  if (!(api = calloc(1, sizeof(DLiteStoragePlugin))))
    FAIL("allocation failure");

  api->name = PyUnicode_DATA(name);
  api->open = opener;
  api->close = closer;
  api->getInstance = instance_getter;
  api->setInstance = instance_setter;
  api->freer = freer;
  api->data = (void *)cls;
  Py_INCREF(cls);

  retval = api;
 fail:
  if (!retval && api) {
    free(api);
    api = NULL;
  }
  Py_XDECREF(name);
  Py_XDECREF(open);
  Py_XDECREF(close);
  Py_XDECREF(get_instance);
  Py_XDECREF(set_instance);

  printf("--> api=%p\n", (void *)api);
  return api;
}
