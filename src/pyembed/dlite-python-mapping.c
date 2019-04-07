/*
  A generic mapping that looks up and loads Python mapping plugins
 */
#include <Python.h>
#include <assert.h>

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-mapping-plugins.h"
#include "dlite-pyembed.h"
#include "dlite-python-mapping.h"


/* Python mapping paths */
static FUPaths mapping_paths;
static int mapping_paths_initialised = 0;
static int mapping_paths_modified = 0;

/* A cache with all loaded plugins */
static PyObject *loaded_mappings = NULL;


/*
  Returns a pointer to Python mapping paths
*/
const FUPaths *dlite_python_mapping_paths(void)
{
  if (!mapping_paths_initialised) {
    if (fu_paths_init(&mapping_paths, "DLITE_PYTHON_PATHS") < 0)
      return dlite_err(1, "cannot load initialise DLITE_PYTHON_PATHS"), NULL;
    mapping_paths_initialised = 1;
    mapping_paths_modified = 0;
  }
  return &mapping_paths;
}

/*
  Clears Python mapping search path.
*/
void dlite_python_mapping_paths_clear(void)
{
  if (mapping_paths_initialised) {
    fu_paths_deinit(&mapping_paths);
    mapping_paths_initialised = 0;
    mapping_paths_modified = 0;
  }
}

/*
  Inserts `path` into Python mapping paths before position `n`. If
  `n` is negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
*/
int dlite_python_mapping_paths_insert(const char *path, int n)
{
  int stat;
  const FUPaths *paths;
  if (!(paths = dlite_python_mapping_paths())) return -1;
  if ((stat = fu_paths_insert((FUPaths *)paths, path, n)))
    mapping_paths_modified = 1;
  return stat;
}

/*
  Appends `path` to Python mapping paths.
  Returns the index of the newly appended element or -1 on error.
*/
int dlite_python_mapping_paths_append(const char *path)
{
  int stat;
  const FUPaths *paths;
  if (!(paths = dlite_python_mapping_paths())) return -1;
  if ((stat = fu_paths_append((FUPaths *)paths, path)))
    mapping_paths_modified = 1;
  return stat;
}

/*
  Removes path index `n` to Python mapping paths.
  Returns non-zero on error.
*/
int dlite_python_mapping_paths_remove(int n)
{
  int stat;
  const FUPaths *paths;
  if (!(paths = dlite_python_mapping_paths())) return -1;
  if ((stat = fu_paths_remove((FUPaths *)paths, n)))
    mapping_paths_modified = 1;
  return stat;
}

/*
  Returns a pointer to the current Python mapping plugin search path
  or NULL on error.
*/
const char **dlite_python_mapping_paths_get(void)
{
  const FUPaths *paths;
  if (!(paths = dlite_python_mapping_paths())) return NULL;
  return fu_paths_get((FUPaths *)paths);
}


/*
  Loads all Python mappings (if needed).

  Returns a borrowed reference to a list of mapping plugins (casted to
  void *) or NULL on error.
*/
void *dlite_python_mapping_load(void)
{
  if (!loaded_mappings || mapping_paths_modified) {
    const FUPaths *paths;
    if (loaded_mappings) dlite_python_mapping_unload();
    if (!(paths = dlite_python_mapping_paths())) return NULL;
    loaded_mappings = dlite_pyembed_load_plugins((FUPaths *)paths,
                                                 "DLiteMappingBase");
  }
  return (void *)loaded_mappings;
}

/* Unloads all currently loaded mappings. */
void dlite_python_mapping_unload(void)
{
  if (loaded_mappings) {
    Py_DECREF(loaded_mappings);
    loaded_mappings = NULL;
  }
}


/*
   Wraps Python method map() into a DLite Mapper.
 */
static DLiteInstance *mapper(const DLiteMappingPlugin *api,
                             const DLiteInstance **instances, int n)
{
  const char *classname;
  PyObject *map=NULL, *ret=NULL;
  PyObject *plugin = (PyObject *)api->data;
  assert(plugin);

  dlite_errclr();
  UNUSED(instances);
  UNUSED(n);

  printf("\n*** python_mapper(n=%d)\n", n);

  if (!(classname = dlite_pyembed_classname(plugin)))
    dlite_warnx("cannot get class name for plugin %p", (void *)plugin);

  if (!(map = PyObject_GetAttrString(plugin, "map")))
    FAIL1("plugin '%s' has no method: 'map'", classname);
  if (!PyCallable_Check(map))
    FAIL1("attribute 'map' of plugin '%s' is not callable", classname);
  if (!(ret = PyObject_CallFunctionObjArgs(map, NULL))) {
    dlite_pyembed_err(1, "error calling %s.map()", classname);
    goto fail;
  }



 fail:
  Py_XDECREF(ret);
  Py_XDECREF(map);

  return NULL;
}

/*
  Free's internal resources in `api`.
*/
static void freer(DLiteMappingPlugin *api)
{
  Py_XDECREF(api->data);
  free(api);
}


/*
  Returns API provided by mapping plugin `name` implemented in Python.

  Default cost is 25.
*/
const DLiteMappingPlugin *get_dlite_mapping_api(const char *name)
{
  int i, cost=25;
  DLiteMappingPlugin *api=NULL, *retval=NULL;
  PyObject *mappings=NULL, *plugin=NULL;
  PyObject *out_uri=NULL, *in_uris=NULL, *map=NULL, *pcost=NULL;
  const char **input_uris=NULL, *classname=NULL;

  if (!(mappings = dlite_python_mapping_load())) goto fail;
  assert(PyList_Check(mappings));

  /* Look up mapping with given name */
  for (i=0; i < (int)PyList_Size(mappings); i++) {
    PyObject *pname=NULL;
    plugin = PyList_GetItem(mappings, i);
    assert(plugin);
    if (!(classname = dlite_pyembed_classname(plugin)))
      dlite_warnx("cannot get class name for plugin %p", (void *)plugin);

    else if (!(pname = PyObject_GetAttrString(plugin, "name")))
      dlite_warnx("plugin '%s' has no attribute: 'name'", classname);
    else if (!PyUnicode_Check(pname))
      dlite_warnx("attribute 'name' of plugin '%s' is not a string", classname);
    else if (strcmp(name, PyUnicode_DATA(pname)) == 0) {
      Py_DECREF(pname);
      break;
    }
    Py_XDECREF(pname);
  }
  if (i >= PyList_Size(mappings)) goto fail;

  if (!(out_uri = PyObject_GetAttrString(plugin, "output_uri")))
    FAIL1("plugin '%s' has no attribute: 'output_uri'", classname);
  if (!PyUnicode_Check(out_uri))
    FAIL1("attribute 'output_uri' of plugin '%s' is not a string", classname);

  if (!(in_uris = PyObject_GetAttrString(plugin, "input_uris")))
    FAIL1("plugin '%s' has no attribute: 'input_uris'", classname);
  if (!PySequence_Check(in_uris))
    FAIL1("attribute 'input_uris' of plugin '%s' is not a sequence", classname);
  if (!(input_uris = calloc(PySequence_Length(in_uris), sizeof(char *))))
    FAIL("allocation failure");
  for (i=0; i < PySequence_Length(in_uris); i++) {
    PyObject *in_uri = PySequence_GetItem(in_uris, i);
    if (!in_uri || !PyUnicode_Check(in_uri)) {
      Py_XDECREF(in_uri);
      FAIL2("item %d of attribute 'input_uris' of plugin '%s' is not a string",
            i, classname);
    }
    input_uris[i] = PyUnicode_DATA(in_uri);
    Py_DECREF(in_uri);
  }

  if (!(map = PyObject_GetAttrString(plugin, "map")))
    FAIL1("plugin '%s' has no method: 'map'", classname);
  if (!PyCallable_Check(map))
    FAIL1("attribute 'map' of plugin '%s' is not callable", classname);

  if ((pcost = PyObject_GetAttrString(plugin, "cost")) && PyLong_Check(pcost))
    cost = PyLong_AsLong(pcost);

  if (!(api = calloc(1, sizeof(DLiteMappingPlugin))))
    FAIL("allocation failure");

  api->name = PyUnicode_DATA(name);
  api->output_uri = PyUnicode_DATA(out_uri);
  api->ninput = PySequence_Length(in_uris);
  api->input_uris = input_uris;
  api->mapper = mapper;
  api->freer = freer;
  api->cost = cost;
  api->data = (void *)plugin;
  Py_INCREF(plugin);

  retval = api;
 fail:
  if (!retval && api) free(api);
  Py_XDECREF(name);
  Py_XDECREF(out_uri);
  Py_XDECREF(in_uris);
  Py_XDECREF(map);
  Py_XDECREF(pcost);
  if (input_uris) free(input_uris);
  return retval;
}
