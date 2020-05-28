#include <stdarg.h>

#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-pyembed.h"


static int python_initialized = 0;

/* Initialises the embedded Python environment. */
void dlite_pyembed_initialise(void)
{
  wchar_t *progname;
  if (!python_initialized) {
    if (!(progname = Py_DecodeLocale("dlite", NULL))) {
      dlite_err(1, "allocation/decoding failure");
      return;
    }
    Py_SetProgramName(progname);
    PyMem_RawFree(progname);
    Py_Initialize();
    python_initialized = 1;
  }
}

/* Finalises the embedded Python environment.  Returns non-zero on error. */
int dlite_pyembed_finalise(void)
{
  int status=0;
  if (python_initialized) {
    status = Py_FinalizeEx();
    python_initialized = 0;
  } else {
    return dlite_errx(1, "cannot finalize Python before it is initialized");
  }
  return status;
}


/*
  Returns a static pointer to the class name of python object cls or
  NULL on error.
*/
const char *dlite_pyembed_classname(PyObject *cls)
{
  const char *classname=NULL;
  PyObject *name=NULL, *sname=NULL;
  if ((name = PyObject_GetAttrString(cls, "__name__")) &&
      (sname = PyObject_Str(name)))
    classname = PyUnicode_AsUTF8(sname);
  Py_XDECREF(name);
  Py_XDECREF(sname);
  return classname;
}


/*
  Reports and resets Python error.

  If an Python error has occured, an error message is appended to `msg`,
  containing the type, value and traceback.

  Returns `eval`.
 */
int dlite_pyembed_err(int eval, const char *msg, ...)
{
  int stat;
  va_list ap;
  va_start(ap, msg);
  stat = dlite_pyembed_verr(eval, msg, ap);
  va_end(ap);
  return stat;
}

/*
  Like dlite_pyembed_err() but takes a `va_list` as input.
 */
int dlite_pyembed_verr(int eval, const char *msg, va_list ap)
{
  if (PyErr_Occurred()) {
    char errmsg[4096];
    PyObject *type, *value, *tb=NULL;

    errmsg[0] = '\0';
    PyErr_Fetch(&type, &value, &tb);
    PyErr_NormalizeException(&type, &value, &tb);
    assert(type && value);

    /* Try to get traceback info from traceback.format_exception()... */
    if (tb) {
      PyObject *module_name=NULL, *module=NULL, *pfunc=NULL, *val=NULL,
        *sep=NULL, *str=NULL;
      if ((module_name = PyUnicode_FromString("traceback")) &&
          (module = PyImport_Import(module_name)) &&
          (pfunc = PyObject_GetAttrString(module, "format_exception")) &&
          PyCallable_Check(pfunc) &&
          (val = PyObject_CallFunctionObjArgs(pfunc, type, value, tb, NULL)) &&
          PySequence_Check(val) &&
          (sep = PyUnicode_FromString("")) &&
          (str = PyUnicode_Join(val, sep)) &&
          PyUnicode_Check(str) &&
          PyUnicode_GET_LENGTH(str) > 0)
        PyOS_snprintf(errmsg, sizeof(errmsg), "%s\n%s",
                      msg, (char *)PyUnicode_AsUTF8(str));
      Py_XDECREF(str);
      Py_XDECREF(sep);
      Py_XDECREF(val);
      Py_XDECREF(pfunc);
      Py_XDECREF(module);
      Py_XDECREF(module_name);
    }

    /* ...otherwise try to report error without traceback... */
    if (!errmsg[0]) {
      PyObject *name=NULL, *sname=NULL, *svalue=NULL;
      if ((name = PyObject_GetAttrString(type, "__name__")) &&
          (sname = PyObject_Str(name)) &&
          PyUnicode_Check(sname) &&
          (svalue = PyObject_Str(value)) &&
          PyUnicode_Check(svalue))
        PyOS_snprintf(errmsg, sizeof(errmsg), "%s: %s: %s",
                      msg, (char *)PyUnicode_AsUTF8(sname),
                      (char *)PyUnicode_AsUTF8(svalue));
      Py_XDECREF(svalue);
      Py_XDECREF(sname);
      Py_XDECREF(name);
    }

    /* ...otherwise skip Python error info */
    if (!errmsg[0])
      PyOS_snprintf(errmsg, sizeof(errmsg), "%s: <inaccessible Python error>",
                    msg);

    if (errmsg[0]) msg = errmsg;

    Py_DECREF(type);
    Py_DECREF(value);
    Py_XDECREF(tb);
  }
  return dlite_verrx(eval, msg, ap);
}

/*
  Checks if an Python error has occured.  Returns zero if no error has
  occured.  Otherwise dlite_pyembed_err() is called and non-zero is
  returned.
 */
int dlite_pyembed_err_check(const char *msg, ...)
{
  int stat;
  va_list ap;
  va_start(ap, msg);
  stat = dlite_pyembed_verr_check(msg, ap);
  va_end(ap);
  return stat;
}

/*
  Like dlite_pyembed_err_check() but takes a `va_list` as input.
 */
int dlite_pyembed_verr_check(const char *msg, va_list ap)
{
  /* TODO: can we correlate the return value to Python error type? */
  if (PyErr_Occurred())
    return dlite_pyembed_verr(1, msg, ap);
  return 0;
}



/*
  Loads the Python C extension module "_dlite" and returns the address
  of `symbol`, within this module.  Returns NULL on error or if
  `symbol` cannot be found.
*/
void *dlite_pyembed_get_address(const char *symbol)
{
  PyObject *dlite_name=NULL, *dlite_module=NULL, *dlite_dict=NULL;
  PyObject *_dlite_module=NULL, *_dlite_dict=NULL, *_dlite_file=NULL;
  PyObject *ctypes_name=NULL, *ctypes_module=NULL, *ctypes_dict=NULL;
  PyObject *PyDLL=NULL, *addressof=NULL;
  PyObject *so=NULL, *sym=NULL, *addr=NULL;
  const char *filename=NULL;
  void *ptr=NULL;

  /* Import dlite */
  if (!(dlite_name = PyUnicode_FromString("dlite")) ||
      !(dlite_module = PyImport_Import(dlite_name)))
    FAIL("cannot import Python package: dlite");

  /* Get path to _dlite */
  if (!(dlite_dict = PyModule_GetDict(dlite_module)) ||
      !(_dlite_module = PyDict_GetItemString(dlite_dict, "_dlite")) ||
      !(_dlite_dict = PyModule_GetDict(_dlite_module)) ||
      !(_dlite_file = PyDict_GetItemString(_dlite_dict, "__file__")))
    FAIL("cannot get path to dlite extension module");

  /* Get C path to _dlite */
  if (!PyUnicode_Check(_dlite_file) ||
      !(filename = PyUnicode_AsUTF8(_dlite_file)))
    FAIL("cannot get C path to dlite extension module");

  /* Get PyDLL() from ctypes */
  if (!(ctypes_name = PyUnicode_FromString("ctypes")) ||
      !(ctypes_module = PyImport_Import(ctypes_name)) ||
      !(ctypes_dict = PyModule_GetDict(ctypes_module)) ||
      !(PyDLL = PyDict_GetItemString(ctypes_dict, "PyDLL")))
    FAIL("cannot find PyDLL() in ctypes");

  /* Get addressof() from ctypes */
  if (!(addressof = PyDict_GetItemString(ctypes_dict, "addressof")))
    FAIL("cannot find addressof() in ctypes");

  /* Locate `symbol` in _dlite using ctypes */
  if (!(so = PyObject_CallFunctionObjArgs(PyDLL, _dlite_file, NULL)))
    FAIL1("error calling PyDLL(\"%s\")", (char *)filename);
  if (!(sym = PyObject_GetAttrString(so, symbol)))
    FAIL2("no such symbol in shared object \"%s\": %s", filename, symbol);
  if (!(addr = PyObject_CallFunctionObjArgs(addressof, sym, NULL)))
    FAIL1("error calling ctypes.addressof(\"%s\")", symbol);
  if (!PyLong_Check(addr))
    FAIL2("address of \"%s\" in %s is not a long", symbol, filename);
  ptr = (void *)PyLong_AsLong(addr);

  /* Seems that ctypes.addressof() returns the address where the pointer to
     `symbol` is stored, so we need an extra dereference... */
  if (ptr) ptr = *((void **)ptr);

 fail:
  Py_XDECREF(addr);
  Py_XDECREF(sym);
  Py_XDECREF(so);
  //Py_XDECREF(addressof);      // borrowed reference
  //Py_XDECREF(PyDLL);          // borrowed reference
  //Py_XDECREF(ctypes_dict);    // borrowed reference
  Py_XDECREF(ctypes_module);
  Py_XDECREF(ctypes_name);
  //Py_XDECREF(_dlite_file);    // borrowed reference
  //Py_XDECREF(_dlite_dict);    // borrowed reference
  //Py_XDECREF(_dlite_module);  // borrowed reference
  //Py_XDECREF(dlite_dict);     // borrowed reference
  Py_XDECREF(dlite_module);
  Py_XDECREF(dlite_name);
  return ptr;
}


/*
  Returns a Python representation of dlite instance with given id or NULL
  on error.
*/
PyObject *dlite_pyembed_from_instance(const char *id)
{
  PyObject *pyid=NULL, *dlite_name=NULL, *dlite_module=NULL, *dlite_dict=NULL;
  PyObject *get_instance=NULL, *instance=NULL;

  if (!(pyid = PyUnicode_FromString(id)))
    FAIL("cannot create python string");

  /* Import dlite */
  if (!(dlite_name = PyUnicode_FromString("dlite")) ||
      !(dlite_module = PyImport_Import(dlite_name)))
    FAIL("cannot import Python package: dlite");

  /* Get reference to Python function get_instance() */
  if (!(dlite_dict = PyModule_GetDict(dlite_module)) ||
      !(get_instance = PyDict_GetItemString(dlite_dict, "get_instance")))
    FAIL("no such Python function: dlite.get_instance()");

  /* Call get_instance() */
  if (!(instance = PyObject_CallFunctionObjArgs(get_instance, pyid, NULL)))
    FAIL("failure calling dlite.get_instance()");

 fail:
  Py_XDECREF(pyid);
  Py_XDECREF(dlite_module);
  Py_XDECREF(dlite_name);
  return instance;
}


/*
  Returns a new reference to DLite instance from Python representation
  or NULL on error.

  Since plugins that statically links to dlite will have their own
  global state, dlite_instance_get() will not work.  Instead, this
  function uses the capsule returned by the Python method Instance._c_ptr().
*/
DLiteInstance *dlite_pyembed_get_instance(PyObject *pyinst)
{
  DLiteInstance *inst=NULL;
  PyObject *fcn=NULL, *cap=NULL;
  if (!(fcn = PyObject_GetAttrString(pyinst, "_c_ptr")))
    FAIL("Python instance has no attribute: '_c_ptr'");
  if (!(cap = PyObject_CallObject(fcn, NULL)))
    FAIL("error calling: '_c_ptr'");
  if (!(inst = PyCapsule_GetPointer(cap, NULL)))
    FAIL("cannot get instance pointer from capsule");
  dlite_instance_incref(inst);
 fail:
  Py_XDECREF(cap);
  Py_XDECREF(fcn);
  return inst;
}


/*
  This function loads all Python modules found in `paths` and returns
  a list of plugin objects.

  A Python plugin is a subclass of `baseclassname` that implements the
  expected functionality.

  Returns NULL on error.
 */
PyObject *dlite_pyembed_load_plugins(FUPaths *paths, const char *baseclassname)
{
  char initcode[96];
  const char *path;
  PyObject *baseclass=NULL, *main_module=NULL, *main_dict=NULL;
  PyObject *ppath=NULL, *pfun=NULL, *subclasses=NULL;
  FUIter *iter;

  dlite_errclr();
  dlite_pyembed_initialise();

  /* Inject base class into the __main__ module */
  if (snprintf(initcode, sizeof(initcode), "class %s: pass\n",
               baseclassname) < 0)
    FAIL("failure to create initialisation code for embedded Python "
         "__main__ module");
  if (PyRun_SimpleString(initcode))
    FAIL("failure when running embedded Python __main__ module "
         "initialisation code");

  /* Extract the base class from the __main__ module */
  if (!(main_module = PyImport_AddModule("__main__")))
    FAIL("cannot load the embedded Python __main__ module");
  if (!(main_dict = PyModule_GetDict(main_module)))
    FAIL("cannot access __dict__ of the embedded Python __main__ module");
  if (!(baseclass = PyDict_GetItemString(main_dict, baseclassname)))
    FAIL1("cannot get base class '%s' from the main dict", baseclassname);


  /* Load all modules in `paths` */
  if (!(iter = fu_pathsiter_init(paths, "*.py"))) goto fail;
  while ((path = fu_pathsiter_next(iter))) {
    int stat;
    FILE *fp=NULL;
    char *basename=NULL;
    PyObject *ret;

    if (!(ppath = PyUnicode_FromString(path)))
      FAIL1("cannot create Python string from path: '%s'", path);
    stat = PyDict_SetItemString(main_dict, "__file__", ppath);
    Py_DECREF(ppath);
    if (stat) FAIL("cannot assign path to '__file__' in dict of main module");

    if ((basename = fu_basename(path)) && (fp = fopen(path, "r"))) {
      ret = PyRun_File(fp, basename, Py_file_input, main_dict, main_dict);
      free(basename);
      if (!ret) {
        dlite_pyembed_err(1, "error parsing '%s'", path);
      } else {
        Py_DECREF(ret);
      }
      fclose(fp);
    }
  }
  if (fu_pathsiter_deinit(iter)) goto fail;

  /* Get list of subclasses implementing the plugins */
  if ((pfun = PyObject_GetAttrString(baseclass, "__subclasses__")))
      subclasses = PyObject_CallFunctionObjArgs(pfun, NULL);

 fail:
  Py_XDECREF(pfun);
  return subclasses;
}
