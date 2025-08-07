#include <stdarg.h>

#include "utils/strutils.h"
#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-behavior.h"
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


/* Struct correlating Python exceptions with DLite errors */
typedef struct {
  PyObject *exc;        /* Python exception */
  DLiteErrCode errcode;  /* DLite error */
} ErrorCorrelation;

/* Global state for this module */
typedef struct {
  ErrorCorrelation *errcorr;  /* NULL-terminated array */
  int initialised;            /* Whether DLite pyembed has been initialised */
  PyObject *dlitedict;        /* Cached dlite dictionary */
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


/*
  Return Python exception class corresponding to given DLite error code.
  Returns NULL if `code` is zero.
 */
PyObject *dlite_pyembed_exception(DLiteErrCode code)
{
  switch (code) {
  case dliteSuccess:               return NULL;
  case dliteUnknownError:          break;
  case dliteIOError:               return PyExc_IOError;
  case dliteRuntimeError:          return PyExc_RuntimeError;
  case dliteIndexError:            return PyExc_IndexError;
  case dliteTypeError:             return PyExc_TypeError;
  case dliteDivisionByZeroError:   return PyExc_ZeroDivisionError;
  case dliteOverflowError:         return PyExc_OverflowError;
  case dliteSyntaxError:           return PyExc_SyntaxError;
  case dliteValueError:            return PyExc_ValueError;
  case dliteSystemError:           return PyExc_SystemError;
  case dliteAttributeError:        return PyExc_AttributeError;
  case dliteMemoryError:           return PyExc_MemoryError;
  case dliteNullReferenceError:    break;

  case dliteOSError:               return PyExc_OSError;
  case dliteKeyError:              return PyExc_KeyError;
  case dliteNameError:             return PyExc_NameError;
  case dliteLookupError:           return PyExc_LookupError;
  case dliteParseError:            return PyExc_IOError;     // dup
  case dlitePermissionError:       return PyExc_PermissionError;
  case dliteSerialiseError:        return PyExc_IOError;     // dup
  case dliteUnsupportedError:      break;
  case dliteVerifyError:           break;
  case dliteInconsistentDataError: return PyExc_ValueError;  // dup
  case dliteInvalidMetadataError:  return PyExc_ValueError;  // dup
  case dliteStorageOpenError:      return PyExc_IOError;     // dup
  case dliteStorageLoadError:      return PyExc_IOError;     // dup
  case dliteStorageSaveError:      return PyExc_IOError;     // dup
  case dliteOptionError:           return PyExc_ValueError;  // dup
  case dliteMissingInstanceError:  return PyExc_LookupError; // dup
  case dliteMissingMetadataError:  return PyExc_LookupError; // dup
  case dliteMetadataExistError:    break;
  case dliteMappingError:          break;
  case dliteProtocolError:         break;
  case dlitePythonError:           break;
  case dliteTimeoutError:          return PyExc_TimeoutError;
  case dliteLastError:             break;
  }
  return PyExc_Exception;
}

/* Help function returning a constant pointer to a NULL-terminated
   array of ErrorCorrelation records. */
static const ErrorCorrelation *error_correlations(void)
{
  PyembedGlobals *g = get_globals();
  if (!g->errcorr) {
    int i, code, n=1;
    for (code=-1; code>dliteLastError; code--)
      if (dlite_pyembed_exception(code) != PyExc_Exception) n++;

    if (!(g->errcorr = calloc(n, sizeof(ErrorCorrelation))))
      return dlite_err(dliteMemoryError, "allocation failure"), NULL;

    for (code=-1, i=0; code>dliteLastError; code--) {
      PyObject *exc;
      if ((exc = dlite_pyembed_exception(code)) != PyExc_Exception) {
        g->errcorr[i].exc = exc;
        g->errcorr[i].errcode = code;
        i++;
      }
    }
    assert(i == n-1);
  }
  return g->errcorr;
}

/*
  Initialises the embedded Python environment.

  From DLite v0.6.0, this function will only initialise an new
  internal Python interpreter if there are no initialised
  interpreters in the process.  This means that if DLite is called
  from Python, the plugins will be called from the calling Python
  interpreter.

  This function can be called more than once.
 */
void dlite_pyembed_initialise(void)
{
  PyembedGlobals *g = get_globals();

  if (!g->initialised) {
      g->initialised = 1;

#if defined(HAVE_SETENV)
      if (Py_IsInitialized()) {
      /* Set environment variables from global variables in Python
         starting with "DLITE_" */
      PyObject *maindict = dlite_python_maindict();
      PyObject *key, *value;
      Py_ssize_t pos = 0;

      while (PyDict_Next(maindict, &pos, &key, &value)) {
        if (PyUnicode_Check(key)) {
          const char *ckey = PyUnicode_AsUTF8AndSize(key, NULL);
          assert(ckey);
          if (strncmp(ckey, "DLITE_", 6) == 0) {
            if (PyBool_Check(value)) {
              if (PyObject_IsTrue(value)) setenv(ckey, "", 1);
            } else if (PyLong_Check(value)) {
              long v = PyLong_AsLong(value);
              char cval[32];
              snprintf(cval, sizeof(cval), "%ld", v);
              setenv(ckey, cval, 1);
            } else if (PyUnicode_Check(value)) {
              const char *cval = PyUnicode_AsUTF8AndSize(value, NULL);
              setenv(ckey, cval, 1);
            } else {
              dlite_warnx("Unsupported type for value of global variable `%s`. "
                          "Should be bool, str or int.", ckey);
            }
          }
        }
      }
    }
#endif

    if (!Py_IsInitialized() || !dlite_behavior_get("singleInterpreter")) {
      /*
        Initialise new Python interpreter

        Python 3.8 and later implements the new Python
        Initialisation Configuration.

        More features were added in the following releases,
        like `config.safe_path`, which was added in Python 3.11

        The old Py_SetProgramName() was deprecated in Python 3.11.

        In DLite, we switch to the new Python Initialisation
        Configuration from Python 3.11.
      */
      PyObject *sys=NULL, *sys_path=NULL, *path=NULL;
#if PY_VERSION_HEX >= 0x030b0000  /* Python >= 3.11 */
      /* New Python Initialisation Configuration */
      PyStatus status;
      PyConfig config;
      wchar_t *executable=NULL;

      PyConfig_InitPythonConfig(&config);
      config.isolated = 0;
      config.safe_path = 0;
      config.use_environment = 1;
      config.user_site_directory = 1;

      /* Set executable if VIRTUAL_ENV environment variable is defined */
      char *venv = getenv("VIRTUAL_ENV");
      if (venv) {
#if defined(__MINGW32__) || defined(__MINGW64__)
        char *exec = fu_join(venv, "bin", "python.exe", NULL);
#elif defined(WINDOWS)
        char *exec = fu_join(venv, "Scripts", "python.exe", NULL);
#else
        char *exec = fu_join(venv, "bin", "python", NULL);
#endif
        status = PyConfig_SetBytesString(&config, &config.executable, exec);
        if (PyStatus_Exception(status))
          FAIL1("failed configuring executable: %s", exec);
      }

      /* If dlite is called from a python, reparse arguments to avoid
         that they are stripped off... */
      if (Py_IsInitialized()) {
        int argc=0;
        wchar_t **argv=NULL;
        Py_GetArgcArgv(&argc, &argv);
        config.parse_argv = 1;
        status = PyConfig_SetArgv(&config, argc, argv);
        if (PyStatus_Exception(status))
          FAIL("failed configuring pyembed arguments");
      }

      status = PyConfig_SetBytesString(&config, &config.program_name, "dlite");
      if (PyStatus_Exception(status))
        FAIL("failed configuring pyembed program name");

      status = Py_InitializeFromConfig(&config);
      PyConfig_Clear(&config);
      if (PyStatus_Exception(status))
        FAIL("failed clearing pyembed config");

      if (executable) free(executable);
#else
      /* Old Initialisation */
      wchar_t *progname;

      Py_Initialize();

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
}

/* Finalises the embedded Python environment.  Returns non-zero on error. */
int dlite_pyembed_finalise(void)
{
  int status=0;
  if (Py_IsInitialized()) {
    status = Py_FinalizeEx();
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
  Writes Python error message to `errmsg` (of length `len`) if an
  Python error has occured.

  Resets the Python error indicator.

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
  if (!tb) PyException_SetTraceback(value, tb);

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
    PYFAILCODE(dlitePythonError, "cannot import Python package: dlite");

  /* Get path to _dlite */
  if (!(dlite_dict = PyModule_GetDict(dlite_module)) ||
      !(_dlite_module = PyDict_GetItemString(dlite_dict, "_dlite")) ||
      !(_dlite_dict = PyModule_GetDict(_dlite_module)) ||
      !(_dlite_file = PyDict_GetItemString(_dlite_dict, "__file__")))
    PYFAILCODE(dlitePythonError, "cannot get path to dlite extension module");

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
  PyObject *ppath=NULL, *pfun=NULL, *subclasses=NULL, *lst=NULL;
  PyObject *subclassnames=NULL;
  FUIter *iter;
  int i;
  FILE *fp=NULL;
  size_t errors_pos=0;
  char errors[4098] = "";

  dlite_errclr();
  dlite_pyembed_initialise();

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
    char *stem;

    if ((stem = fu_stem(path))) {
      int stat;
      PyObject *plugindict;

      if (!(plugindict = dlite_python_plugindict(stem))) goto fail;
      if (!(ppath = PyUnicode_FromString(path)))
        FAIL1("cannot create Python string from path: '%s'", path);
      stat = PyDict_SetItemString(plugindict, "__file__", ppath);
      Py_DECREF(ppath);
      if (stat)
        FAIL("cannot assign path to '__file__' in dict of main module");

      size_t n;
      char **q = (failed_paths) ? *failed_paths : NULL;
      for (n=0; q && *q; n++)
        if (strcmp(*(q++), path) == 0) break;
      int in_failed = (q && *q) ? 1 : 0;  // whether loading path has failed

      if (!in_failed) {
        if ((fp = fopen(path, "r"))) {
          PyObject *ret = PyRun_File(fp, path, Py_file_input, plugindict,
                                     plugindict);
          if (!ret) {
            if (failed_paths && failed_len) {
              char **new = strlst_append(*failed_paths, failed_len, path);
              if (!new) FAIL("allocation failure");
              *failed_paths = new;
            }

            int m;
            if (errors_pos < sizeof(errors) &&
                (m = snprintf(errors+errors_pos, sizeof(errors)-errors_pos,
                              "  - %s: (%s): ", stem, path)) > 0)
              errors_pos += m;
            if (errors_pos < sizeof(errors) &&
                (m = dlite_pyembed_errmsg(errors+errors_pos,
                                          sizeof(errors)-errors_pos)) > 0)
              errors_pos += m;
            if (errors_pos < sizeof(errors) &&
                (m = snprintf(errors+errors_pos, sizeof(errors)-errors_pos,
                              "\n")) > 0)
              errors_pos += m;
          }
          Py_XDECREF(ret);
          fclose(fp);
          fp = NULL;
        }
      }
      free(stem);
    }

  }
  if (fu_pathsiter_deinit(iter)) goto fail;

  if (errors[0]) {
    dlite_info("Could not load the following Python plugins:\n%s"
               "You may have to install missing python package(s).\n",
               errors);
  }



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
  if (fp) fclose(fp);
  return subclasses;
}


/*
  Return borrowed reference to a dict object for DLite or NULL on error.

  If the dlite module has been imported, the dlite module `__dict__`
  is returned.  Otherwise a warning is issued and a, possible newly
  created, `__main__._dlite` dict is returned.

  The returned reference is cashed and will therefore always be consistent.

  Use dlite_python_module_dict() if you only want the dlite module dict.
 */
PyObject *dlite_python_dlitedict(void)
{
  PyObject *name=NULL, *module=NULL, *dict=NULL;
  PyembedGlobals *g = get_globals();

  dlite_pyembed_initialise();

  /* Caching the dlitedict */
  if (g->dlitedict) return g->dlitedict;

  if (!(name = PyUnicode_FromString("dlite")))
    FAILCODE(dliteValueError, "invalid string: 'dlite'");

  if (!(module = PyImport_GetModule(name))) {
    PyObject *maindict = dlite_python_maindict();
    if (!maindict) goto fail;
    if (!(dict = PyDict_GetItemString(maindict, "_dlite"))) {
      if (!(dict = PyDict_New()))
        FAILCODE(dlitePythonError, "cannot create dict `__main__._dlite`");
      int stat = PyDict_SetItemString(maindict, "_dlite", dict);
      Py_DECREF(dict);
      if (stat) FAILCODE(dlitePythonError,
                         "cannot insert dict `__main__._dlite`");
      dlite_warnx("dlite not imported.  Created dict `__main__._dlite`");
    }
  } else {
    if (!(dict = PyModule_GetDict(module)))
      FAILCODE(dlitePythonError, "cannot get dlite module dict");
  }

  g->dlitedict = dict;

 fail:
  Py_XDECREF(name);
  Py_XDECREF(module);

  return dict;
}


/*
  Return borrowed reference to a dict serving as a namespace for the
  given plugin.

  The returned dict is accessable from Python as
  `dlite._plugindict[plugin_name]`.  The dict will be created if it
  doesn't already exists.

  Returns NULL on error.
 */
PyObject *dlite_python_plugindict(const char *plugin_name)
{
  PyObject *dlitedict=NULL, *plugindict=NULL, *dict=NULL;

  if (!(dlitedict = dlite_python_dlitedict())) goto fail;

  if (!(plugindict = PyDict_GetItemString(dlitedict, "_plugindict"))) {
    if (!(plugindict = PyDict_New()))
      FAILCODE(dlitePythonError, "cannot create dict `dlite._plugindict`");
    int stat = PyDict_SetItemString(dlitedict, "_plugindict", plugindict);
    Py_DECREF(plugindict);
    if (stat) FAILCODE(dlitePythonError,
                       "cannot insert dict `dlite._plugindict`");
  }

  if (!(dict = PyDict_GetItemString(plugindict, plugin_name))) {
    if (!(dict = PyDict_New()))
      FAILCODE1(dlitePythonError,
               "cannot create dict `dlite._plugindict[%s]`",
               plugin_name);
    int stat = PyDict_SetItemString(plugindict, plugin_name, dict);
    Py_DECREF(dict);
    if (stat) FAILCODE1(dlitePythonError,
                        "cannot insert dict `dlite._plugindict[%s]`",
                        plugin_name);
  }

 fail:
  return dict;
}
