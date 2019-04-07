#include <stdarg.h>

#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-pyembed.h"

static int initialized = 0;

/* Initialises the embedded Python environment. */
void dlite_pyembed_initialise(void)
{
  if (!initialized) {
    Py_Initialize();
    initialized = 1;
  }
}

/* Finalises the embedded Python environment. */
int dlite_pyembed_finalise(void)
{
  if (initialized) {
    Py_Finalize();
    initialized = 0;
  }
  return 0;
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
    classname = PyUnicode_DATA(sname);
  Py_XDECREF(name);
  Py_XDECREF(sname);
  return classname;
}


/*
  Reports and restes Python error.

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
                      msg, (char *)PyUnicode_DATA(str));
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
                      msg, (char *)PyUnicode_DATA(sname),
                      (char *)PyUnicode_DATA(svalue));
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
  if (!(iter = fu_startmatch("*.py", paths))) goto fail;
  while ((path = fu_nextmatch(iter))) {
    int stat;
    FILE *fp=NULL;
    char *basename=NULL;
    PyObject *ret;

    /* Set __main__.__dict__['__file__'] = path */
    if (!(ppath = PyUnicode_FromString(path)))
      FAIL1("cannot create Python string from path: '%s'", path);
    stat = PyDict_SetItemString(main_dict, "__file__", ppath);
    Py_XDECREF(ppath);
    if (stat) FAIL("cannot assign path to '__file__' in dict of main module");

    if ((basename = fu_basename(path)) && (fp = fopen(path, "r"))) {
      ret = PyRun_File(fp, basename, Py_file_input, main_dict, main_dict);
      free(basename);
      if (!ret) {
        //PyErr_Print();  // xxx
        dlite_pyembed_err(1, "error parsing '%s'", path);
      } else {
        Py_DECREF(ret);
      }
    }
  }
  if (fu_endmatch(iter)) goto fail;

  /* Get list of subclasses implementing the plugins */
  if ((pfun = PyObject_GetAttrString(baseclass, "__subclasses__")))
      subclasses = PyObject_CallFunctionObjArgs(pfun, NULL);

 fail:
  Py_XDECREF(pfun);
  return subclasses;
}
