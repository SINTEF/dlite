/* dlite-plugins-python.c -- a DLite plugin for plugins written in python */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
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
} DLitePythonStorage;


/*
  Checks whether a Python error has occured.  If so, it calls dlite_err(),
  cleans the Python error and returns non-zero.  Otherwise zero is returned.
*/

int check_error(void)
{
  int retval = 0;
  PyObject *type, *value, *traceback;
  PyErr_Fetch(&type, &value, &traceback);
  if (type) {
    PyObject *stype = PyObject_Str(type);
    PyObject *svalue = PyObject_Str(value);
    PyObject *straceback = PyObject_Str(traceback);
    PyObject *module = PyImport_ImportModule("traceback");
    retval = 1;
    if (module) {
      PyObject *format_exc = PyObject_GetAttrString(module, "format_exc");
      if (format_exc) {
	PyObject *msg=NULL;
	PyErr_Restore(type, value, traceback);
	msg = PyObject_CallObject(format_exc, NULL);
	if (msg && PyUnicode_Check(msg))
	  retval = dlite_err(1, PyUnicode_AsUTF8(msg));
	Py_XDECREF(msg);
      }
      Py_XDECREF(format_exc);
    }
    Py_XDECREF(module);
    Py_XDECREF(straceback);
    Py_XDECREF(svalue);
    Py_XDECREF(stype);
    if (!retval)
      retval = dlite_err(1, "unknown Python error");
    PyErr_Clear();
  }
  Py_XDECREF(traceback);
  Py_XDECREF(value);
  Py_XDECREF(type);
  return retval;
}


/*
    printf("*** Error:\n  type (%d): ", PyUnicode_Check(stype));
    PyObject_Print(type, stdout, 1);
    printf("\n  value (%d): ", PyUnicode_Check(svalue));
    PyObject_Print(value, stdout, 1);
    printf("\n  traceback (%d): ", PyUnicode_Check(straceback));
    PyObject_Print(traceback, stdout, 1);
    printf("\n");
    retval = dlite_err(1, "%s: %s\n\n%s",
		       PyUnicode_AsUTF8(stype),
		       PyUnicode_AsUTF8(svalue),
		       PyUnicode_AsUTF8(straceback));
    PyErr_Clear();
    Py_XDECREF(stype);
    Py_XDECREF(svalue);
    Py_XDECREF(straceback);
  }
  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(traceback);
  return retval;
}
*/


DLiteStorage *
opener(const DLiteStoragePlugin *api, const char *uri, const char *options)
{
  DLitePythonStorage *s=NULL;
  DLiteStorage *retval=NULL;
  PyObject *obj=NULL, *v=NULL, *writable=NULL;
  PyObject *cls = (PyObject *)api->data;
  const char *classname;

  if (!(classname = dlite_pyembed_classname(cls)))
    dlite_warnx("cannot get class name for storage plugin %s", *((char **)api));

  /* Call method: open() */
  if (!(obj = PyObject_CallObject(cls, NULL)))
    FAIL1("error instantiating %s", classname);
  v = PyObject_CallMethod(obj, "open", "ss", uri, options);
  if (dlite_pyembed_err_check("error calling %s.open()", classname)) goto fail;

  /* Check if the open() method has set attribute `writable` */
  if (PyObject_HasAttrString(obj, "writable"))
    writable = PyObject_GetAttrString(obj, "writable");

  if (!(s = calloc(1, sizeof(DLitePythonStorage))))
    FAIL("Allocation failure");
  s->api = api;
  s->uri = strdup(uri);
  s->options = strdup(options);
  s->writable = (writable) ? PyObject_IsTrue(writable) : 1;
  s->obj = obj;

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
  int retval=0;
  DLitePythonStorage *sp = (DLitePythonStorage *)s;
  PyObject *v = NULL;
  PyObject *class = (PyObject *)s->api->data;
  const char *classname;

  dlite_errclr();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin %s",
		*((char **)s->api));
  v = PyObject_CallMethod(sp->obj, "close", "");
  if (dlite_pyembed_err_check("error calling %s.close()", classname))
    retval = 1;
  Py_XDECREF(v);
  return retval;
}

/*
  Returns a new instance from `uuid` in storage `s`.  NULL is returned
  on error.
 */
DLiteInstance *loader(const DLiteStorage *s, const char *uuid)
{
  DLitePythonStorage *sp = (DLitePythonStorage *)s;
  PyObject *pyuuid = PyUnicode_FromString(uuid);
  DLiteInstance *inst = NULL;
  PyObject *class = (PyObject *)s->api->data;
  const char *classname;

  dlite_errclr();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin %s",
		*((char **)s->api));
  printf("instance_getter(%s)\n", uuid);
  PyObject *v = PyObject_CallMethod(sp->obj, "load", "O", pyuuid);
  if (dlite_pyembed_err_check("error calling %s.load()", classname))
    goto fail;
  assert(v);
  /* Here we have an issue with storage plugins being statically linked */
  printf("--- v = ");
  PyObject_Print(v, stdout, 0);
  printf("\n");

  printf("inst: %p\n", (void *)dlite_instance_get(uuid));

  printf("calling dlite_pyembed_get_instance(%p)\n", (void *)v);
  if (!(inst = dlite_pyembed_get_instance(v))) goto fail;

  {
    DLiteInstance *inst2 = dlite_instance_get(inst->uuid);
    printf("*** inst=%p, inst2=%p\n", (void *)inst, (void *)inst2);
    if (inst2) dlite_instance_decref(inst2);
  }

  printf("done\n");
 fail:
  Py_XDECREF(pyuuid);
  Py_XDECREF(v);
  return inst;
}

/*
  Stores instance `inst` to storage `s`.  Returns non-zero on error.
*/
int saver(DLiteStorage *s, const DLiteInstance *inst)
{
  DLitePythonStorage *sp = (DLitePythonStorage *)s;
  PyObject *pyinst = dlite_pyembed_from_instance(inst->uuid);
  PyObject *v = NULL;
  int retval = 1;
  PyObject *class = (PyObject *)s->api->data;
  const char *classname;
  dlite_errclr();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin %s",
		*((char **)s->api));
  v = PyObject_CallMethod(sp->obj, "save", "O", pyinst);
  if (dlite_pyembed_err_check("error calling %s.save()", classname)) goto fail;
  retval = 0;
 fail:
  Py_XDECREF(pyinst);
  Py_XDECREF(v);
  return retval;
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
  PyObject *open=NULL, *close=NULL, *load=NULL, *save=NULL;
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
  if (PyObject_HasAttrString(cls, "name"))
    name = PyObject_GetAttrString(cls, "name");
  else
    name = PyUnicode_FromString(classname);
  if (!PyUnicode_Check(name))
    FAIL1("attribute 'name' (or '__name__') of '%s' is not a string",
          (char *)PyUnicode_AsUTF8(name));

  if (!(open = PyObject_GetAttrString(cls, "open")))
    FAIL1("'%s' has no method: 'open'", classname);
  if (!PyCallable_Check(open))
    FAIL1("attribute 'open' of '%s' is not callable", classname);

  if (!(close = PyObject_GetAttrString(cls, "close")))
    FAIL1("'%s' has no method: 'close'", classname);
  if (!PyCallable_Check(close))
    FAIL1("attribute 'close' of '%s' is not callable", classname);

  if (PyObject_HasAttrString(cls, "load")) {
    load = PyObject_GetAttrString(cls, "load");
    if (!PyCallable_Check(load))
      FAIL1("attribute 'load' of '%s' is not callable", classname);
  }

  if (PyObject_HasAttrString(cls, "save")) {
    save = PyObject_GetAttrString(cls, "save");
    if (!PyCallable_Check(save))
      FAIL1("attribute 'save' of '%s' is not callable", classname);
  }

  if (!load && !save)
    FAIL1("expect either method 'load()' or 'save()' to be defined in '%s'",
	  classname);

  if (!(api = calloc(1, sizeof(DLiteStoragePlugin))))
    FAIL("allocation failure");

  api->name = PyUnicode_AsUTF8(name);
  api->open = opener;
  api->close = closer;
  api->loadInstance = loader;
  api->saveInstance = saver;
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
  Py_XDECREF(load);
  Py_XDECREF(save);

  printf("--> api=%p\n", (void *)api);
  return api;
}
