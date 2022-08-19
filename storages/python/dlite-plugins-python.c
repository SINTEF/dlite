/* dlite-plugins-python.c -- a DLite plugin for plugins written in python */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "Python.h"

#include "utils/boolean.h"
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
  Opens `location` and returns a newly created storage for it.

  The `location` and `options` arguments are passed on to the open
  method in Python.

  Returns NULL on error.
 */
DLiteStorage *
opener(const DLiteStoragePlugin *api, const char *location,
       const char *options)
{
  DLitePythonStorage *s=NULL;
  DLiteStorage *retval=NULL;
  PyObject *obj=NULL, *v=NULL, *readable=NULL, *writable=NULL, *generic=NULL;
  PyObject *cls = (PyObject *)api->data;
  const char *classname;

  if (!(classname = dlite_pyembed_classname(cls)))
    dlite_warnx("cannot get class name for storage plugin %s", api->name);

  /* Call method: open() */
  if (!(obj = PyObject_CallObject(cls, NULL)))
    FAIL1("error instantiating %s", classname);
  v = PyObject_CallMethod(obj, "open", "ss", location, options);
  if (dlite_pyembed_err_check("error calling %s.open()", classname)) goto fail;

  /* Check if the open() method has set attribute `writable` */
  if (PyObject_HasAttrString(obj, "readable"))
    readable = PyObject_GetAttrString(obj, "readable");
  if (PyObject_HasAttrString(obj, "writable"))
    writable = PyObject_GetAttrString(obj, "writable");
  if (PyObject_HasAttrString(obj, "generic"))
    generic = PyObject_GetAttrString(obj, "generic");

  if (!(s = calloc(1, sizeof(DLitePythonStorage))))
    FAIL("Allocation failure");
  s->api = api;

  if (readable && !PyObject_IsTrue(readable))  // default: redable
    s->flags &= ~dliteReadable;
  else
    s->flags |= dliteReadable;

  if (writable && !PyObject_IsTrue(writable))  // default: writable
    s->flags &= ~dliteWritable;
  else
    s->flags |= dliteWritable;

  if (generic && PyObject_IsTrue(generic))     // default: not generic
    s->flags |= dliteGeneric;
  else
    s->flags &= ~dliteGeneric;

  s->obj = obj;
  s->idflag = dliteIDTranslateToUUID;

  retval = (DLiteStorage *)s;
 fail:
  if (s && !retval) {
    Py_XDECREF(s->obj);
    free(s);
  }
  Py_XDECREF(v);
  Py_XDECREF(readable);
  Py_XDECREF(writable);
  Py_XDECREF(generic);

  return retval;
}


/*
  Closes storage `s`.  Returns non-zero on error.
 */
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
  Py_DECREF(sp->obj);
  return retval;
}


/*
  Returns a new instance from `id` in storage `s`.  NULL is returned
  on error.
 */
DLiteInstance *loader(const DLiteStorage *s, const char *id)
{
  DLitePythonStorage *sp = (DLitePythonStorage *)s;
  PyObject *pyuuid;
  DLiteInstance *inst = NULL;
  PyObject *class = (PyObject *)s->api->data;
  const char *classname;

  if (id) {
    pyuuid = PyUnicode_FromString(id);
  } else {
    Py_INCREF(Py_None);
    pyuuid = Py_None;
  }

  dlite_errclr();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin %s",
		*((char **)s->api));
  PyObject *v = PyObject_CallMethod(sp->obj, "load", "O", pyuuid);
  Py_DECREF(pyuuid);

  if (v) {
    inst = dlite_pyembed_get_instance(v);
    Py_DECREF(v);
  } else
    dlite_pyembed_err(1, "error calling %s.load()", classname);

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
static void freeapi(PluginAPI *api)
{
  DLiteStoragePlugin *a = (DLiteStoragePlugin *)api;

  free((char *)a->name);
  Py_XDECREF(a->data);
  free(a);
}


/* Struct returned by iterCreate(). */
typedef struct {
  PyObject *v;           /* iterator returned by Python method queue() */
  //const char *pattern;   /* pattern */
  const char *classname; /* class name */
} Iter;

/*
  Free's iterator created with IterCreate().
*/
void iterFree(void *iter)
{
  Iter *i = (Iter *)iter;
  Py_XDECREF(i->v);
  //if (i->pattern) free((char *)i->pattern);
  free(i);
}

/*
  Returns a new iterator over all instances in storage `s` who's metadata
  URI matches `pattern`.
 */
void *iterCreate(const DLiteStorage *s, const char *pattern)
{
  DLitePythonStorage *sp = (DLitePythonStorage *)s;
  void *retval=NULL;
  Iter *iter = NULL;
  PyObject *class = (PyObject *)s->api->data;
  const char *classname;
  dlite_errclr();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin %s",
		*((char **)s->api));

  if (!(iter = calloc(1, sizeof(Iter)))) FAIL("allocation failure");

  iter->v = PyObject_CallMethod(sp->obj, "queue", "s", pattern);
  if (dlite_pyembed_err_check("error calling %s.queue()", classname)) goto fail;
  if (!PyIter_Check(iter->v))
    FAIL1("method %s.queue() does not return a iterator object", classname);

  iter->classname = classname;

  retval = (void *)iter;
 fail:
  if (!retval && iter) iterFree(iter);
  return retval;
}

/*
  Writes the UUID to buffer pointed to by `buf` of the next instance
  in `iter`, where `iter` is an iterator created with IterCreate().

  Returns zero on success, 1 if there are no more UUIDs to iterate
  over and a negative number on other errors.
 */
int iterNext(void *iter, char *buf)
{
  const char *uuid;
  int retval = -1;
  Iter *i = (Iter *)iter;
  PyObject *next = PyIter_Next((PyObject *)i->v);

  if (dlite_pyembed_err_check("error iterating over %s.queue()",
                              i->classname)) goto fail;
  if (next) {
    if (!PyUnicode_Check(next))
      FAIL1("generator method %s.queue() should return a string", i->classname);
    if (!(uuid = PyUnicode_AsUTF8(next)) || strlen(uuid) != DLITE_UUID_LENGTH)
      FAIL1("generator method %s.queue() should return a uuid", i->classname);
    memcpy(buf, uuid, DLITE_UUID_LENGTH+1);
    retval = 0;
  } else {
    retval = 1;
  }
 fail:
  Py_XDECREF(next);
  return retval;
}


/*
  Returns API provided by storage plugin `name` implemented in Python.
*/
DSL_EXPORT const DLiteStoragePlugin *
get_dlite_storage_plugin_api(void *state, int *iter)
{
  int n;
  DLiteStoragePlugin *api=NULL, *retval=NULL;
  PyObject *storages=NULL, *cls=NULL, *name=NULL;
  PyObject *open=NULL, *close=NULL, *queue=NULL, *load=NULL, *save=NULL;
  const char *classname=NULL;

  dlite_globals_set(state);

  if (!(storages = dlite_python_storage_load())) goto fail;
  assert(PyList_Check(storages));
  n = (int)PyList_Size(storages);

  /* get class implementing the plugin API */
  dlite_errclr();
  if (*iter < 0 || *iter >= n)
    FAIL1("API iterator index is out of range: %d", *iter);
  cls = PyList_GetItem(storages, *iter);
  assert(cls);
  if (*iter < n - 1) (*iter)++;

  /* get classname for error messages */
  if (!(classname = dlite_pyembed_classname(cls)))
    dlite_warnx("cannot get class name for storage plugin");

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

  if (PyObject_HasAttrString(cls, "queue")) {
    queue = PyObject_GetAttrString(cls, "queue");
    if (!PyCallable_Check(queue))
      FAIL1("attribute 'queue' of '%s' is not callable", classname);
  }

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

  api->name = strdup(PyUnicode_AsUTF8(name));
  api->freeapi = freeapi;
  api->open = opener;
  api->close = closer;
  if (queue) {
    api->iterCreate = iterCreate;
    api->iterNext = iterNext;
    api->iterFree = iterFree;
  }
  api->loadInstance = loader;
  api->saveInstance = saver;
  api->data = (void *)cls;
  Py_INCREF(cls);

  retval = api;
 fail:
  if (!retval && api) free(api);
  Py_XDECREF(name);
  Py_XDECREF(open);
  Py_XDECREF(close);
  Py_XDECREF(load);
  Py_XDECREF(save);

  return retval;
}
