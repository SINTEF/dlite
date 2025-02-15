/*
  A generic mapping that looks up and loads Python mapping plugins
 */
#include <Python.h>
#include <assert.h>
#include <stdlib.h>

/* Python pulls in a lot of defines that conflicts with utils/config.h */
#define SKIP_UTILS_CONFIG_H

#include "config-paths.h"

#include "utils/sha3.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-mapping-plugins.h"
#include "dlite-pyembed.h"
#include "dlite-python-singletons.h"
#include "dlite-python-mapping.h"

#define GLOBALS_ID "dlite-python-mapping-id"


/* Prototype for function converting `inst` to a Python object.
   Returns a new reference or NULL on error. */
typedef PyObject *(*InstanceConverter)(DLiteInstance *inst);


/* Global variables for dlite-storage-plugins */
typedef struct {
  /* Python mapping paths */
  FUPaths mapping_paths;
  int mapping_paths_initialised;
  unsigned char mapping_plugin_path_hash[32];
  PyObject *loaded_mappings;  /* A cache with all loaded plugins */
  char **failed_paths;  /* NULL-terminated array of paths to storages
                           that fail to load. */
  size_t failed_len;    /* Allocated length of `failed_paths`. */
} Globals;


/* Frees global state for this module - called by atexit() */
static void free_globals(void *globals)
{
  Globals *g = globals;
  dlite_python_mapping_paths_clear();

  /* Note that we cannot call `Py_XDECREF(g->loaded_mappings)` at this stage,
     since that requires that GIL is held, but GIL is released during atexit().
  */
  free(g);
}

/* Return a pointer to global state for this module */
static Globals *get_globals(void)
{
  Globals *g = dlite_globals_get_state(GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(Globals))))
      return dlite_err(dliteMemoryError, "allocation failure"), NULL;
    dlite_globals_add_state(GLOBALS_ID, g, free_globals);
  }
  return g;
}


/*
  Returns a pointer to Python mapping paths
*/
FUPaths *dlite_python_mapping_paths(void)
{
  Globals *g;
  if (!(g = get_globals())) return NULL;

  if (!g->mapping_paths_initialised) {
    int s;
    if (fu_paths_init(&g->mapping_paths,
                      "DLITE_PYTHON_MAPPING_PLUGIN_DIRS") < 0)
      return dlite_err(1, "cannot initialise "
                       "DLITE_PYTHON_MAPPING_PLUGIN_DIRS"), NULL;

    fu_paths_set_platform(&g->mapping_paths, dlite_get_platform());

    if (dlite_use_build_root())
      s = fu_paths_extend(&g->mapping_paths,
                          dlite_PYTHON_MAPPING_PLUGINS, NULL);
    else
      s = fu_paths_extend_prefix(&g->mapping_paths, dlite_pkg_root_get(),
                                 DLITE_PYTHON_MAPPING_PLUGIN_DIRS, NULL);
    if (s < 0) return dlite_err(1, "error initialising dlite python mapping "
                                "plugin dirs"), NULL;
    g->mapping_paths_initialised = 1;
    memset(g->mapping_plugin_path_hash, 0, sizeof(g->mapping_plugin_path_hash));

    /* Make sure that dlite DLLs are added to the library search path */
    dlite_add_dll_path();

    /* Be kind with memory leak software and free memory at exit... */
    //atexit(dlite_python_mapping_paths_clear);
  }

  return &g->mapping_paths;
}

/*
  Clears Python mapping search path.
*/
void dlite_python_mapping_paths_clear(void)
{
  Globals *g;
  if (!(g = get_globals())) return;

  if (g->mapping_paths_initialised) {
    fu_paths_deinit(&g->mapping_paths);
    memset(g->mapping_plugin_path_hash, 0, sizeof(g->mapping_plugin_path_hash));
    g->mapping_paths_initialised = 0;
  }
}

/*
  Inserts `path` into Python mapping paths before position `n`. If
  `n` is negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
*/
int dlite_python_mapping_paths_insert(const char *path, int n)
{
  const FUPaths *paths;
  if (!(paths = dlite_python_mapping_paths())) return -1;
  return fu_paths_insert((FUPaths *)paths, path, n);
}

/*
  Appends `path` to Python mapping paths.
  Returns the index of the newly appended element or -1 on error.
*/
int dlite_python_mapping_paths_append(const char *path)
{
  const FUPaths *paths;
  if (!(paths = dlite_python_mapping_paths())) return -1;
  return fu_paths_append((FUPaths *)paths, path);
}

/*
  Removes path number `index` from the Python mapping paths.
  Returns non-zero on error.
*/
int dlite_python_mapping_paths_remove_index(int index)
{
  const FUPaths *paths;
  if (!(paths = dlite_python_mapping_paths())) return -1;
  return fu_paths_remove_index((FUPaths *)paths, index);
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
  FUPaths *paths;
  FUIter *iter;
  const char *path;
  const unsigned char *hash;
  sha3_context c;
  PyObject *mappingbase;
  Globals *g;
  if (!(g = get_globals())) return NULL;

  if (!(mappingbase = dlite_python_mapping_base())) return NULL;
  if (!(paths = dlite_python_mapping_paths())) return NULL;
  if (!(iter = fu_pathsiter_init(paths, "*.py"))) return NULL;
  sha3_Init256(&c);
  while ((path = fu_pathsiter_next(iter)))
    sha3_Update(&c, path, strlen(path));
  hash = sha3_Finalize(&c);
  fu_pathsiter_deinit(iter);
  if (memcmp(g->mapping_plugin_path_hash, hash,
             sizeof(g->mapping_plugin_path_hash)) != 0) {
    if (g->loaded_mappings) dlite_python_mapping_unload();
    g->loaded_mappings = dlite_pyembed_load_plugins((FUPaths *)paths,
                                                    mappingbase,
                                                    &g->failed_paths,
                                                    &g->failed_len);
    memcpy(g->mapping_plugin_path_hash, hash,
           sizeof(g->mapping_plugin_path_hash));
  }
  return (void *)g->loaded_mappings;
}

/* Unloads all currently loaded mappings. */
void dlite_python_mapping_unload(void)
{
  Globals *g;
  if (!(g = get_globals())) return;

  if (g->loaded_mappings) {
    Py_DECREF(g->loaded_mappings);
    g->loaded_mappings = NULL;
  }
}


/*
   Wraps Python method map() into a DLite Mapper.
 */
static DLiteInstance *mapper(const DLiteMappingPlugin *api,
                             const DLiteInstance **instances, int n)
{
  int i;
  const char *classname, *uuid;
  DLiteInstance *inst=NULL;
  PyObject *map=NULL, *insts=NULL, *outinst=NULL, *pyuuid=NULL;
  PyObject *plugin = (PyObject *)api->data;
  assert(plugin);
  dlite_errclr();

  /* Creates Python list of input instances */
  if (!(insts = PyList_New(n)))
    FAIL("failed to create list");
  for (i=0; i<n; i++) {
    PyObject *pyinst;

    if (!(pyinst = dlite_pyembed_from_instance(instances[i]->uuid))) goto fail;
    PyList_SetItem(insts, i, pyinst);
  }

  /* Call Python map() method */
  if (!(classname = dlite_pyembed_classname(plugin)))
    dlite_warnx("cannot get class name for plugin %p", (void *)plugin);
  if (!(map = PyObject_GetAttrString(plugin, "map")))
    FAIL1("plugin '%s' has no method: 'map'", classname);
  if (!PyCallable_Check(map))
    FAIL1("attribute 'map' of plugin '%s' is not callable", classname);
  if (!(outinst = PyObject_CallFunctionObjArgs(map, plugin, insts, NULL))) {
    dlite_pyembed_err(1, "error calling %s.map()", classname);
    goto fail;
  }

  /* Extract uuid from returned `outinst` object and get corresponding
     C instance */
  if (!(pyuuid = PyObject_GetAttrString(outinst, "uuid")))
    FAIL("output instance has no such attribute: uuid");
  if (!PyUnicode_Check(pyuuid) || !(uuid = PyUnicode_AsUTF8(pyuuid)))
    FAIL("cannot convert uuid");
  if (!(inst = dlite_instance_get(uuid)))
    FAIL1("no such instance: %s", uuid);

 fail:

  Py_XDECREF(pyuuid);
  Py_XDECREF(outinst);
  Py_XDECREF(insts);
  Py_XDECREF(map);
  for (i=0; i<n; i++) dlite_instance_decref((DLiteInstance *)instances[i]);
  return inst;
}

/*
  Free's internal resources in `api`.
*/
static void freeapi(PluginAPI *api)
{
  DLiteMappingPlugin *p = (DLiteMappingPlugin *)api;
  int i;
  free(p->name);
  free((char *)p->output_uri);
  for (i=0; i<p->ninput; i++) free((char *)(p->input_uris[i]));
  free((char **)p->input_uris);
  Py_XDECREF(p->data);
  free(p);
}


/* Forward declaration */
const DLiteMappingPlugin *get_dlite_mapping_api(void *state, int *iter);

/*
  Returns pointer to next Python mapping plugin (casted to void *) or
  NULL on error.

  `state` should be a pointer to the global state.

  At the first call to this function, `*iter` should be initialised to zero.
  If there are more APIs, `*iter` will be increased by one.
*/
const void *dlite_python_mapping_next(void *state, int *iter)
{
  return get_dlite_mapping_api(state, iter);
}

/*
  Returns Python mapping plugin (casted to void *) with given name or
  NULL if no matches can be found.
 */
const void *dlite_python_mapping_get_api(const char *name)
{
  const DLiteMappingPlugin *api;
  int iter1=0, iter2;
  void *state = dlite_globals_get();
  do {
    iter2 = iter1;
    if (!(api = get_dlite_mapping_api(state, &iter1))) break;
    if (strcmp(api->name, name) == 0) return api;
  } while (iter1 > iter2);
  return NULL;
}


/*
  Returns API provided by mapping plugin `name` implemented in Python.

  `state` should be a pointer to the global state obtained with
  dlite_globals_get().

  At the first call to this function, `*iter` should be initialised to zero.
  If there are more APIs, `*iter` will be increased by one.

  Default cost is 25.
*/
const DLiteMappingPlugin *get_dlite_mapping_api(void *state, int *iter)
{
  int i, n, cost=25;
  DLiteMappingPlugin *api=NULL, *retval=NULL;
  PyObject *mappings=NULL, *cls=NULL;
  PyObject *name=NULL, *out_uri=NULL, *in_uris=NULL, *map=NULL, *pcost=NULL;
  const char *output_uri=NULL, **input_uris=NULL, *classname=NULL;
  char *apiname=NULL;

  dlite_globals_set(state);

  if (!(mappings = dlite_python_mapping_load())) goto fail;
  assert(PyList_Check(mappings));
  n = (int)PyList_Size(mappings);
  if (n == 0) return NULL;

  /* get class implementing the plugin API */
  if (*iter < 0 || *iter >= n)
    FAIL1("Mapping API iterator index is out of range: %d", *iter);
  cls = PyList_GetItem(mappings, *iter);
  assert(cls);
  if (*iter < n - 1) (*iter)++;

  /* get classname for error messages */
  if (!(classname = dlite_pyembed_classname(cls)))
    dlite_warnx("cannot get class name for API");

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
    FAILCODE(dliteMemoryError, "allocation failure");
  for (i=0; i < PySequence_Length(in_uris); i++) {
    PyObject *in_uri = PySequence_GetItem(in_uris, i);
    if (!in_uri || !PyUnicode_Check(in_uri)) {
      Py_XDECREF(in_uri);
      FAIL2("item %d of attribute 'input_uris' of '%s' is not a string",
            i, classname);
    }
    input_uris[i] = strdup(PyUnicode_AsUTF8(in_uri));
    Py_DECREF(in_uri);
  }

  if (!(map = PyObject_GetAttrString(cls, "map")))
    FAIL1("'%s' has no method: 'map'", classname);
  if (!PyCallable_Check(map))
    FAIL1("attribute 'map' of '%s' is not callable", classname);

  if ((pcost = PyObject_GetAttrString(cls, "cost")) && PyLong_Check(pcost))
    cost = PyLong_AsLong(pcost);

  if (!(api = calloc(1, sizeof(DLiteMappingPlugin))))
    FAILCODE(dliteMemoryError, "allocation failure");

  apiname = strdup(PyUnicode_AsUTF8(name));
  output_uri = strdup(PyUnicode_AsUTF8(out_uri));

  api->name = apiname;
  api->freeapi = freeapi;
  api->output_uri = output_uri;
  api->ninput = (int)PySequence_Length(in_uris);
  api->input_uris = input_uris;
  api->mapper = mapper;
  api->cost = cost;
  api->data = (void *)cls;
  Py_INCREF(cls);

  retval = api;
 fail:
  Py_XDECREF(name);
  Py_XDECREF(out_uri);
  Py_XDECREF(in_uris);
  Py_XDECREF(map);
  Py_XDECREF(pcost);
  if (!retval) {
    if (apiname) free(apiname);
    if (output_uri) free((char *)output_uri);
    if (input_uris) free((char **)input_uris);
    if (api) free(api);
  }
  return retval;
}
