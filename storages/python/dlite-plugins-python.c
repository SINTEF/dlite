/* dlite-plugins-python.c -- a DLite plugin for plugins written in python */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

/* Make sure that #-formats for PyObject_CallMethod() uses Py_ssize_t for
   Python 3.9 and older...
   See https://docs.python.org/3/c-api/arg.html#strings-and-buffers */
#define PY_SSIZE_T_CLEAN
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


/* Standard addition to error message for errors occuring within a plugin */
static char *failmsg()
{
  if (!getenv("DLITE_PYDEBUG"))
    return
      "\n"
      "   To see error messages from Python storages, please rerun with the\n"
      "   DLITE_PYDEBUG environment variable set.\n"
      "   For example: `export DLITE_PYDEBUG=`\n"
      "\n";
  else
    return "";
}


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

  PyErr_Clear();

  if (!(classname = dlite_pyembed_classname(cls)))
    dlite_warnx("cannot get class name for storage plugin '%s'", api->name);

  /* Call method: open() */
  if (!(obj = PyObject_CallObject(cls, NULL)))
    FAILCODE1(dliteStorageOpenError, "error instantiating Python plugin '%s'",
              classname);

  v = PyObject_CallMethod(obj, "open", "ss", location, options);
  if (dlite_pyembed_err_check("calling open() in Python plugin '%s'%s",
                              classname, failmsg()))
    goto fail;

  /* Check if the open() method has set attribute `writable` */
  if (PyObject_HasAttrString(obj, "readable"))
    readable = PyObject_GetAttrString(obj, "readable");
  if (PyObject_HasAttrString(obj, "writable"))
    writable = PyObject_GetAttrString(obj, "writable");
  if (PyObject_HasAttrString(obj, "generic"))
    generic = PyObject_GetAttrString(obj, "generic");

  if (!(s = calloc(1, sizeof(DLitePythonStorage))))
    FAILCODE(dliteMemoryError, "Allocation failure");
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
    dlite_warnx("cannot get class name for storage plugin '%s'", s->api->name);

  /* Return if close() is not defined */
  if (!PyObject_HasAttrString(sp->obj, "close")) return retval;

  v = PyObject_CallMethod(sp->obj, "close", "");
  if (dlite_pyembed_err_check("calling close() in Python plugin '%s'%s",
                              classname, failmsg()))
    retval = 1;
  Py_XDECREF(v);
  Py_DECREF(sp->obj);
  return retval;
}


/*
  Flushes storage `s`.  Returns non-zero on error.
 */
int flusher(DLiteStorage *s)
{
  int retval=0;
  DLitePythonStorage *sp = (DLitePythonStorage *)s;
  PyObject *v = NULL;
  PyObject *class = (PyObject *)s->api->data;
  const char *classname;

  dlite_errclr();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin '%s'", s->api->name);

  /* Return if flush() is not defined */
  if (!PyObject_HasAttrString(sp->obj, "flush")) return retval;

  v = PyObject_CallMethod(sp->obj, "flush", "");
  if (dlite_pyembed_err_check("calling flush() in Python plugin '%s'%s",
                              classname, failmsg()))
    retval = 1;
  Py_XDECREF(v);
  return retval;
}


/*
  Returns a malloc'ed string documenting storage `s` or NULL on error.

  It combines the class documentation with the documentation of the open()
  method.
 */
char *helper(const DLiteStoragePlugin *api)
{
  PyObject *v=NULL, *pyclassdoc=NULL, *open=NULL, *pyopendoc=NULL;
  PyObject *class = (PyObject *)api->data;
  const char *classname, *classdoc=NULL, *opendoc=NULL;
  char *doc=NULL;
  Py_ssize_t n=0, clen=0, olen=0, i, newlines=0;

  dlite_errclr();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin '%s'", api->name);

  if (PyObject_HasAttrString(class, "__doc__")) {
    if (!(pyclassdoc = PyObject_GetAttrString(class, "__doc__")))
      FAILCODE1(dliteAttributeError, "cannot access %s.__doc__", classname);
    if (!(classdoc = PyUnicode_AsUTF8AndSize(pyclassdoc, &clen)))
      FAILCODE1(dliteAttributeError, "cannot read %s.__doc__", classname);
    for (i=n-1; i>0 && isspace(classdoc[i]) && newlines<2; i--) newlines++;
  }

  if (PyObject_HasAttrString(class, "open")) {
    if (!(open = PyObject_GetAttrString(class, "open")))
      FAILCODE1(dliteAttributeError, "cannot access %s.open()", classname);
    if (PyObject_HasAttrString(open, "__doc__")) {
      if (!(pyopendoc = PyObject_GetAttrString(open, "__doc__")))
        FAILCODE1(dliteAttributeError, "cannot access %s.open.__doc__",
                  classname);
      if (!(opendoc = PyUnicode_AsUTF8AndSize(pyopendoc, &olen)))
        FAILCODE1(dliteAttributeError, "cannot read %s.open.__doc__",
                  classname);
    }
  }
  assert(newlines >= 0);
  assert(newlines <= 2);
  if (!(doc = malloc(clen + 2 - newlines + olen + 1)))
    FAILCODE(dliteMemoryError, "allocation failure");
  if (clen) {
    memcpy(doc+n, classdoc, clen);
    n += clen;
  }
  if (clen && olen) {
    memcpy(doc+n, "\n\n", 2 - newlines);
    n += 2 - newlines;
  }
  if (olen) {
    memcpy(doc+n, opendoc, olen);
    n += olen;
  }
  doc[n++] = '\0';
 fail:
  Py_XDECREF(v);
  Py_XDECREF(pyclassdoc);
  Py_XDECREF(open);
  Py_XDECREF(pyopendoc);
  return doc;
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
    dlite_warnx("cannot get class name for storage plugin '%s'", s->api->name);
  PyObject *v = PyObject_CallMethod(sp->obj, "load", "O", pyuuid);
  Py_DECREF(pyuuid);
  if (v) {
    inst = dlite_pyembed_get_instance(v);
    Py_DECREF(v);
  } else
    dlite_pyembed_err(1, "calling load() in Python plugin '%s'%s",
                      classname, failmsg());

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
    dlite_warnx("cannot get class name for storage plugin '%s'", s->api->name);
  v = PyObject_CallMethod(sp->obj, "save", "O", pyinst);
  if (dlite_pyembed_err_check("calling save() in Python plugin '%s'%s",
                              classname, failmsg()))
    goto fail;
  retval = 0;
 fail:
  Py_XDECREF(pyinst);
  Py_XDECREF(v);
  return retval;
}


/*
  Stores instance `inst` to storage `s`.  Returns non-zero on error.
*/
int deleter(DLiteStorage *s, const char *id)
{
  DLitePythonStorage *sp = (DLitePythonStorage *)s;
  PyObject *v = NULL;
  int retval = 1;
  PyObject *class = (PyObject *)s->api->data;
  const char *classname;
  char uuid[DLITE_UUID_LENGTH+1];
  dlite_errclr();
  if (dlite_get_uuid(uuid, id) < 0) goto fail;
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin '%s'", s->api->name);
  v = PyObject_CallMethod(sp->obj, "delete", "s", uuid);
  if (dlite_pyembed_err_check("calling delete() in Python plugin '%s'%s",
                              classname, failmsg()))
    goto fail;
  retval = 0;
 fail:
  Py_XDECREF(v);
  return retval;
}


/*
  Loads instance with given id from bytes object.
 */
DLiteInstance *memloader(const DLiteStoragePlugin *api,
                         const unsigned char *buf, size_t size,
                         const char *id, const char *options)
{
  DLiteInstance *inst = NULL;
  PyObject *v = NULL;
  PyObject *class = (PyObject *)api->data;
  const char *classname;
  PyErr_Clear();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin '%s'", api->name);

  /* Keep backward compatibility with Python plugins that do not have an
     `options` argument in their `from_bytes()` method. */
  if (options)
    v = PyObject_CallMethod(class, "from_bytes", "y#ss",
                            (const char *)buf, (Py_ssize_t) size, id, options);
  else
    v = PyObject_CallMethod(class, "from_bytes", "y#s",
                            (const char *)buf, (Py_ssize_t) size, id);

  if (dlite_pyembed_err_check("calling from_bytes() in Python plugin '%s'",
                              classname)) {
    Py_XDECREF(v);
    return NULL;
  }
  if (v) {
    inst = dlite_pyembed_get_instance(v);
    Py_DECREF(v);
  } else
    dlite_pyembed_err(1, "calling from_bytes() in Python plugin '%s'%s",
                      classname, failmsg());
  return inst;
}

/*
  Saves instance to bytes object.
 */
int memsaver(const DLiteStoragePlugin *api, unsigned char *buf, size_t size,
             const DLiteInstance *inst, const char *options)
{
  Py_ssize_t length = 0;
  char *buffer = NULL;
  PyObject *pyinst = dlite_pyembed_from_instance(inst->uuid);
  PyObject *v = NULL;
  int retval = dliteStorageSaveError;
  PyObject *class = (PyObject *)api->data;
  const char *classname;
  dlite_errclr();
  if (!pyinst) goto fail;
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin '%s'", api->name);

  /* Keep backward compatibility with Python plugins that do not have an
     `options` argument in their `to_bytes()` method. */
  if (options)
    v = PyObject_CallMethod(class, "to_bytes", "Os", pyinst, options);
  else
    v = PyObject_CallMethod(class, "to_bytes", "O", pyinst);

  if (dlite_pyembed_err_check("calling to_bytes() in Python plugin '%s'%s",
                              classname, failmsg()))
    goto fail;
  if (PyBytes_Check(v)) {
    if (PyBytes_AsStringAndSize(v, &buffer, &length)) goto fail;
  } else if (PyByteArray_Check(v)) {
    if ((length = PyByteArray_Size(v)) < 0) goto fail;
    if (!(buffer = PyByteArray_AsString(v))) goto fail;
  } else {
    dlite_errx(dliteStorageSaveError,
               "%s.to_bytes() must return bytes-like object", classname);
    goto fail;
  }
  assert(length > 0);
  memcpy(buf, buffer, (size > (size_t)length) ? (size_t)length : size);
  retval = (int)length;
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
  PyObject *v;           /* iterator returned by Python method query() */
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
  const char *classname, *name="query";
  dlite_errclr();
  if (!(classname = dlite_pyembed_classname(class)))
    dlite_warnx("cannot get class name for storage plugin '%s'", s->api->name);

  if (!(iter = calloc(1, sizeof(Iter))))
    FAILCODE(dliteMemoryError, "allocation failure");

  /* Due to typo, fallback to old method name: queue() */
  if (!PyObject_HasAttrString(sp->obj, name) &&
      dlite_behavior_get("storageQuery") == 0) name = "queue";
  if (!PyObject_HasAttrString(sp->obj, name))
    FAIL1("no such method: %s.query()", classname);

  iter->v = PyObject_CallMethod(sp->obj, name, "s", pattern);
  if (dlite_pyembed_err_check("calling %s() in Python plugin '%s'%s",
                              name, classname, failmsg()))
    goto fail;
  if (!PyIter_Check(iter->v))
    FAIL1("method %s.%s() does not return a iterator object", classname, name);

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

  if (dlite_pyembed_err_check("error iterating over %s.query()",
                              i->classname)) goto fail;
  if (next) {
    if (!PyUnicode_Check(next))
      FAIL1("generator method %s.query() should return a string", i->classname);
    if (!(uuid = PyUnicode_AsUTF8(next)) || strlen(uuid) != DLITE_UUID_LENGTH)
      FAIL1("generator method %s.query() should return a uuid", i->classname);
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
  PyObject *open=NULL, *close=NULL, *query=NULL, *load=NULL, *save=NULL,
    *flush=NULL, *delete=NULL, *memload=NULL, *memsave=NULL;
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
    dlite_warnx("cannot get class name for storage plugin: '%s'", api->name);

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

  if (PyObject_HasAttrString(cls, "close")) {
    close = PyObject_GetAttrString(cls, "close");
    if (!PyCallable_Check(close))
      FAIL1("attribute 'close' of '%s' is not callable", classname);
  }

  if (PyObject_HasAttrString(cls, "flush")) {
    flush = PyObject_GetAttrString(cls, "flush");
    if (!PyCallable_Check(flush))
      FAIL1("attribute 'flush' of '%s' is not callable", classname);
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

  if (PyObject_HasAttrString(cls, "delete")) {
    delete = PyObject_GetAttrString(cls, "delete");
    if (!PyCallable_Check(delete))
      FAIL1("attribute 'delete' of '%s' is not callable", classname);
  }

  if (PyObject_HasAttrString(cls, "from_bytes")) {
    memload = PyObject_GetAttrString(cls, "from_bytes");
    if (!PyCallable_Check(memload))
      FAIL1("attribute 'from_bytes' of '%s' is not callable", classname);
  }

  if (PyObject_HasAttrString(cls, "to_bytes")) {
    memsave = PyObject_GetAttrString(cls, "to_bytes");
    if (!PyCallable_Check(memsave))
      FAIL1("attribute 'to_bytes' of '%s' is not callable", classname);
  }

  /* Due to typo, fallback to old method name: queue() */
  char *name = "query";
  if (!PyObject_HasAttrString(sp->obj, name) &&
      dlite_behavior_get("storageQuery") == 0) name = "queue";
  if (PyObject_HasAttrString(cls, name)) {
    query = PyObject_GetAttrString(cls, name);
    if (!PyCallable_Check(query))
      FAIL2("attribute '%s' of '%s' is not callable", name, classname);
  }

  if (!(api = calloc(1, sizeof(DLiteStoragePlugin))))
    FAILCODE(dliteMemoryError, "allocation failure");

  api->name = strdup(PyUnicode_AsUTF8(name));
  api->freeapi = freeapi;
  api->open = opener;
  api->close = closer;
  api->flush = flusher;
  api->help = helper;
  if (query) {
    api->iterCreate = iterCreate;
    api->iterNext = iterNext;
    api->iterFree = iterFree;
  }
  api->loadInstance = loader;
  api->saveInstance = saver;
  api->deleteInstance = deleter;

  api->memLoadInstance = memloader;
  api->memSaveInstance = memsaver;

  api->data = (void *)cls;
  Py_INCREF(cls);

  retval = api;
 fail:
  if (!retval && api) free(api);
  Py_XDECREF(name);
  Py_XDECREF(open);
  Py_XDECREF(close);
  Py_XDECREF(flush);
  Py_XDECREF(load);
  Py_XDECREF(save);
  Py_XDECREF(delete);
  Py_XDECREF(memload);
  Py_XDECREF(memsave);
  Py_XDECREF(query);

  return retval;
}
