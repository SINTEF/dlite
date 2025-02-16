/*
  A generic protocol that looks up and loads Python protocol plugins
 */
#include <Python.h>
#include <assert.h>
#include <stdlib.h>

/* Python pulls in a lot of defines that conflicts with utils/config.h */
#define SKIP_UTILS_CONFIG_H

#include "config-paths.h"

#include "utils/fileutils.h"
#include "dlite.h"
#include "config.h"
#include "dlite-macros.h"
#include "dlite-misc.h"
#include "dlite-python-protocol.h"
//#include "dlite-pyembed.h"
//#include "dlite-python-singletons.h"

#define GLOBALS_ID "dlite-python-protocol-id"


/* Global variables for dlite-storage-plugins */
typedef struct {
  FUPaths protocol_paths;
  int protocol_paths_initialised;
} Globals;


/* Frees global state for this module - called by atexit() */
static void free_globals(void *globals)
{
  Globals *g = globals;
  dlite_python_protocol_paths_clear();
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
  Returns a pointer to Python protocol paths
*/
FUPaths *dlite_python_protocol_paths(void)
{
  Globals *g;
  if (!(g = get_globals())) return NULL;

  if (!g->protocol_paths_initialised) {
    int s;
    if (fu_paths_init(&g->protocol_paths,
                      "DLITE_PYTHON_PROTOCOL_PLUGIN_DIRS") < 0)
      return dlite_err(1, "cannot initialise "
                       "DLITE_PYTHON_PROTOCOL_PLUGIN_DIRS"), NULL;

    fu_paths_set_platform(&g->protocol_paths, dlite_get_platform());

    if (dlite_use_build_root())
      s = fu_paths_extend(&g->protocol_paths,
                          dlite_PYTHON_PROTOCOL_PLUGINS, NULL);
    else
      s = fu_paths_extend_prefix(&g->protocol_paths, dlite_pkg_root_get(),
                                 DLITE_PYTHON_PROTOCOL_PLUGIN_DIRS, NULL);
    if (s < 0) return dlite_err(1, "error initialising dlite python protocol "
                                "plugin dirs"), NULL;
    g->protocol_paths_initialised = 1;

    /* Make sure that dlite DLLs are added to the library search path */
    dlite_add_dll_path();

    /* Be kind with memory leak software and free memory at exit... */
    //atexit(dlite_python_protocol_paths_clear);
  }

  return &g->protocol_paths;
}

/*
  Clears Python protocol search path.
*/
void dlite_python_protocol_paths_clear(void)
{
  Globals *g;
  if (!(g = get_globals())) return;

  if (g->protocol_paths_initialised) {
    fu_paths_deinit(&g->protocol_paths);
    g->protocol_paths_initialised = 0;
  }
}
