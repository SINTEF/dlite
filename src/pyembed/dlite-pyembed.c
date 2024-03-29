#include <stdarg.h>

#include "utils/strutils.h"
#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-pyembed.h"
#include "dlite-python-storage.h"
#include "dlite-python-mapping.h"
#include "dlite-python-singletons.h"
#include "config-paths.h"

#define GLOBALS_ID "dlite-pyembed-globals"

/* Get rid of MSVS warnings */
#if defined WIN32 || defined _WIN32 || defined __WIN32__
# pragma warning(disable: 4273 4996)
#endif


static int python_initialized = 0;


/* Struct correlating Python exceptions with DLite errors */
typedef struct {
  PyObject *exc;        /* Python exception */
  DLiteErrCode errcode;  /* DLite error */
} ErrorCorrelation;

/* Global state for this module */
typedef struct {
  ErrorCorrelation *errcorr;  /* NULL-terminated array */
} PyembedGlobals;


/* Free global state for this module */
static void free_globals(void *globals)
{
  PyembedGlobals *g = (PyembedGlobals *)globals;;
  if (g->errcorr) free(g->errcorr);
  free(g);
}

/* Return a pointer to global state for this module */
static PyembedGlobals *get_globals(void)
{
  PyembedGlobals *g = dlite_globals_get_state(GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(PyembedGlobals))))
      return dlite_err(dliteMemoryError, "allocation failure"), NULL;
    dlite_globals_add_state(GLOBALS_ID, g, free_globals);
  }
  return g;
}

/* Help function returning a constant pointer to a NULL-terminated
   array of ErrorCorrelation records. */
static const ErrorCorrelation *error_correlations(void)
{
  PyembedGlobals *g = get_globals();
  if (!g->errcorr) {
    ErrorCorrelation corr[] = {
      {PyExc_KeyError, dliteKeyError},
      {PyExc_MemoryError, dliteMemoryError},
      {PyExc_AttributeError, dliteAttributeError},
      {PyExc_SystemError, dliteSystemError},
      {PyExc_ValueError, dliteValueError},
      {PyExc_SyntaxError, dliteSyntaxError},
      {PyExc_OverflowError, dliteOverflowError},
      {PyExc_ZeroDivisionError, dliteDivisionByZero},
      {PyExc_TypeError, dliteTypeError},
      {PyExc_IndexError, dliteIndexError},
      {PyExc_RuntimeError, dliteRuntimeError},
      {PyExc_IOError, dliteIOError},
      {NULL, 0}
    };
    if (!(g->errcorr = malloc(sizeof(corr))))
      return dlite_err(dliteMemoryError, "allocation failure"), NULL;
    memcpy(g->errcorr, corr, sizeof(corr));
  }
  return g->errcorr;
}

/* Initialises the embedded Python environment. */
void dlite_pyembed_initialise(void)
{
  if (!python_initialized) {
    PyObject *sys=NULL, *sys_path=NULL, *path=NULL;

    Py_Initialize();
    python_initialized = 1;

    /*
      Python 3.8 and later implements the new Python
      Initialisation Configuration.

      More features were added in the following releases,
      like `config.safe_path`, which was added in Python 3.11

      The old Py_SetProgramName() was deprecated in Python 3.11.

      In DLite, we switch to the new Python Initialisation
      Configuration from Python 3.11.
    */
#if PY_VERSION_HEX >= 0x030b0000  /* Python >= 3.11 */
    /* New Python Initialisation Configuration */
    PyStatus status;
    PyConfig config;

    PyConfig_InitPythonConfig(&config);
    config.isolated = 0;
    config.safe_path = 0;
    config.use_environment = 1;
    config.user_site_directory = 1;

    /* If dlite is called from a python, reparse arguments to avoid
       that they are stripped off...
       Aren't we initialising a new interpreter? */
    int argc=0;
    wchar_t **argv=NULL;
    Py_GetArgcArgv(&argc, &argv);
    config.parse_argv = 1;
    status = PyConfig_SetArgv(&config, argc, argv);
    if (PyStatus_Exception(status))
      FAIL("failed configuring pyembed arguments");

    status = PyConfig_SetBytesString(&config, &config.program_name, "dlite");
    if (PyStatus_Exception(status))
      FAIL("failed configuring pyembed program name");

    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status))
      FAIL("failed clearing pyembed config");
#else
    /* Old Initialisation */
    wchar_t *progname;

    if (!(progname = Py_DecodeLocale("dlite", NULL))) {
      dlite_err(1, "allocation/decoding failure");
      return;
    }
    Py_SetProgramName(progname);
    PyMem_RawFree(progname);
 #endif

    if (dlite_use_build_root()) {
      if (!(sys = PyImport_ImportModule("sys")))
        FAIL("cannot import sys");
      if (!(sys_path = PyObject_GetAttrString(sys, "path")))
        FAIL("cannot access sys.path");
      if (!PyList_Check(sys_path))
        FAIL("sys.path is not a list");
      if (!(path = PyUnicode_FromString(dlite_PYTHONPATH)))
        FAIL("cannot create python object for dlite_PYTHONPATH");
      if (PyList_Insert(sys_path, 0, path))
        FAIL1("cannot insert %s into sys.path", dlite_PYTHONPATH);
    }
  fail:
    Py_XDECREF(sys);
    Py_XDECREF(sys_path);
    Py_XDECREF(path);
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
  Return DLite error code given Python exception type.
 */
DLiteErrCode dlite_pyembed_errcode(PyObject *type)
{
  const ErrorCorrelation *corr = error_correlations();
  if (!type) return dliteSuccess;
  while (corr->exc) {
    if (PyErr_GivenExceptionMatches(type, corr->exc))
      return corr->errcode;
    corr++;
  }
  return dliteUnknownError;
}

/*
  Return Python exception class corresponding to given DLite error code.
  Returns NULL if `code` is zero.
 */
PyObject *dlite_pyembed_exception(DLiteErrCode code)
{
  const ErrorCorrelation *corr = error_correlations();
  if (!code) return NULL;
  while (corr->exc) {
    if (code == corr->errcode) return corr->exc;
    corr++;
  }
  return PyExc_Exception;
}


/*
  Writes Python error message to `errmsg` (of length `len`) if an
  Python error has occured.

  On return the The Python error indicator is reset.

  Returns 0 if no error has occured.  Otherwise return the number of
  bytes written to, or would have been written to `errmsg` if it had
  been large enough.  On error -1 is returned.
*/
int dlite_pyembed_errmsg(char *errmsg, size_t errlen)
{
  PyObject *type, *value, *tb;
  int n=-1;

  PyErr_Fetch(&type, &value, &tb);
  if (!type) return 0;
  PyErr_NormalizeException(&type, &value, &tb);

  /* Try to create a error message from Python using the treaceback package */
  if (errmsg) {
    PyObject *module_name=NULL, *module=NULL, *pfunc=NULL, *val=NULL,
      *sep=NULL, *str=NULL;
    errmsg[0] = '\0';
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
      n = PyOS_snprintf(errmsg, errlen, "%s", (char *)PyUnicode_AsUTF8(str));
    Py_XDECREF(str);
    Py_XDECREF(sep);
    Py_XDECREF(val);
    Py_XDECREF(pfunc);
    Py_XDECREF(module);
    Py_XDECREF(module_name);
  }

  /* ...otherwise try to report error without traceback... */
  if (errmsg && n < 0) {
    PyObject *name=NULL, *sname=NULL, *svalue=NULL;
    if ((name = PyObject_GetAttrString(type, "__name__")) &&
        (sname = PyObject_Str(name)) &&
        PyUnicode_Check(sname) &&
        (svalue = PyObject_Str(value)) &&
        PyUnicode_Check(svalue))
      n = PyOS_snprintf(errmsg, errlen, "%s: %s",
                        (char *)PyUnicode_AsUTF8(sname),
                        (char *)PyUnicode_AsUTF8(svalue));

    Py_XDECREF(svalue);
    Py_XDECREF(sname);
    Py_XDECREF(name);
  }

  /* ...otherwise fallback to write to sys.stderr.
     We also do this if the DLITE_PYDEBUG environment variable is set. */
  if ((errmsg && n < 0) || getenv("DLITE_PYDEBUG")) {
    PyErr_Restore(type, value, tb);
    PySys_WriteStderr("\n");
    PyErr_PrintEx(0);
    PySys_WriteStderr("\n");
  } else {
    Py_DECREF(type);
    Py_DECREF(value);
    Py_XDECREF(tb);
  }

  return (errmsg) ? n : 0;
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
  char errmsg[4096], *p=errmsg;
  int m = sizeof(errmsg);
  int n = vsnprintf(errmsg, sizeof(errmsg), msg, ap);
  if (n > 0) {
    p += n;
    m -= n;
    n = snprintf(p, m, ": ");
    if (n > 0) {
      p += n;
      m -= n;
    }
  }
  dlite_pyembed_errmsg(p, m);
  return dlite_errx(eval, "%s", errmsg);
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
  PyObject *err;
  //PyGILState_STATE state = PyGILState_STATE();
  err = PyErr_Occurred();
  //PyGILState_Release(state);
  if (err) {
    int eval = dlite_pyembed_errcode(err);
    return dlite_pyembed_verr(eval, msg, ap);
  }
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
  const char *fname=NULL;
  char *filename=NULL;
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
      !(fname = PyUnicode_AsUTF8(_dlite_file)))
    FAIL("cannot get C path to dlite extension module");
  if (!(filename = fu_nativepath(fname, NULL, 0, NULL)))
    FAIL1("cannot convert path: '%s'", fname);

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
  ptr = PyLong_AsVoidPtr(addr);

  /* Seems that ctypes.addressof() returns the address where the pointer to
     `symbol` is stored, so we need an extra dereference... */
  if (ptr) ptr = (void *)*((void **)ptr);

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
  if (filename) free(filename);
  return ptr;
}


/*
  Returns a Python representation of dlite instance with given id or
  Py_None if `id` is NULL.

  On error NULL is returned.
*/
PyObject *dlite_pyembed_from_instance(const char *id)
{
  PyObject *pyid=NULL, *dlite_name=NULL, *dlite_module=NULL, *dlite_dict=NULL;
  PyObject *get_instance=NULL, *instance=NULL;

  if (!id)
    Py_RETURN_NONE;

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

  A Python plugin is a subclass of `baseclass` that implements the
  expected functionality.

  If `failed_paths` is given, it should be a pointer to a
  NULL-terminated array of pointers to paths to plugins that failed to
  load.  In case a plugin fails to load, this array will be updated.

  If `failed_paths` is given, `failed_len` must also be given. It is a
  pointer to the allocated length of `*failed_paths`.

  Returns NULL on error.
 */
PyObject *dlite_pyembed_load_plugins(FUPaths *paths, PyObject *baseclass,
                                     char ***failed_paths, size_t *failed_len)
{
  const char *path;
  PyObject *main_dict, *ppath=NULL, *pfun=NULL, *subclasses=NULL, *lst=NULL;
  PyObject *subclassnames=NULL;
  FUIter *iter;
  int i;
  char errors[4098] = "";

  dlite_errclr();
  dlite_pyembed_initialise();
  if (!(main_dict = dlite_python_maindict())) goto fail;

  /* Get list of initial subclasses and corresponding set subclassnames */
  if ((pfun = PyObject_GetAttrString(baseclass, "__subclasses__")))
      subclasses = PyObject_CallFunctionObjArgs(pfun, NULL);
  Py_XDECREF(pfun);
  if (!(subclassnames = PySet_New(NULL))) FAIL("cannot create empty set");
  for (i=0; i < PyList_Size(subclasses); i++) {
    PyObject *item = PyList_GetItem(subclasses, i);
    PyObject *name = PyObject_GetAttrString(item, "__name__");
    if (name) {
      if (PySet_Contains(subclassnames, name) == 0 &&
          PySet_Add(subclassnames, name)) FAIL("cannot add class name to set");
    } else {
      FAIL("cannot get name attribute from class");
    }
    Py_XDECREF(name);
    name = NULL;
  }

  /* Load all modules in `paths` */
  if (!(iter = fu_pathsiter_init(paths, "*.py"))) goto fail;
  while ((path = fu_pathsiter_next(iter))) {
    int stat;
    FILE *fp=NULL;
    char *basename=NULL;

    if (!(ppath = PyUnicode_FromString(path)))
      FAIL1("cannot create Python string from path: '%s'", path);
    stat = PyDict_SetItemString(main_dict, "__file__", ppath);
    Py_DECREF(ppath);
    if (stat) FAIL("cannot assign path to '__file__' in dict of main module");

    if ((basename = fu_basename(path))) {
      size_t n;
      char **q = (failed_paths) ? *failed_paths : NULL;
      for (n=0; q && *q; n++)
        if (strcmp(*(q++), path) == 0) break;
      int in_failed = (q && *q) ? 1 : 0;  // whether loading path has failed

      if (!in_failed) {
        if ((fp = fopen(path, "r"))) {
          PyObject *ret = PyRun_File(fp, basename, Py_file_input, main_dict,
                                     main_dict);
          if (!ret) {

            if (failed_paths && failed_len) {
              char **new = strlst_append(*failed_paths, failed_len, path);
              if (!new) FAIL("allocation failure");
              *failed_paths = new;
            }

            dlite_pyembed_errmsg(NULL, 0);
            fclose(fp);
          }
          Py_XDECREF(ret);
        }
      }
      free(basename);
    }

  }
  if (fu_pathsiter_deinit(iter)) goto fail;

  if (errors[0])
    dlite_warn("Could not load the following Python plugins:\n%s"
               "   You might have to install corresponding python "
               "package(s).\n",
               errors);

  /* Append new subclasses to the list of Python plugins that will be
     returned */
  if ((pfun = PyObject_GetAttrString(baseclass, "__subclasses__")))
      lst = PyObject_CallFunctionObjArgs(pfun, NULL);
  Py_XDECREF(pfun);
  for (i=0; i < PyList_Size(lst); i++) {
    PyObject *item = PyList_GetItem(lst, i);
    PyObject *name = PyObject_GetAttrString(item, "__name__");
    if (name) {
      if (PySet_Contains(subclassnames, name) == 0) {
        if (PySet_Add(subclassnames, name))
          FAIL("cannot add class name to set of subclass names");
        if (PyList_Append(subclasses, item))
          FAIL("cannot append subclass to list of subclasses");
      }
    } else {
      FAIL("cannot get name attribute from class");
    }
    Py_XDECREF(name);
    name = NULL;
  }

 fail:
  Py_XDECREF(lst);
  Py_XDECREF(subclassnames);
  return subclasses;
}
