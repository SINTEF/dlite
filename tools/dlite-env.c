/* dlite-env.c -- run command in dlite environment */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#include "utils/compat.h"
#include "utils/compat/getopt.h"
#include "utils/err.h"
#include "utils/execprocess.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "config-paths.h"


void help()
{
  char **p, *msg[] = {
    "Usage: dlite-env [OPTIONS] [--] [COMMAND args...]",
    "Runs COMMAND with environment variables correctly set up for dlite.",
    "  -b, --build         Prepend paths with build directories.",
    "  -e, --empty         Do not include current environment.",
    "  -h, --help          Prints this help and exit.",
    "  -i, --no-install    Do not include install paths.",
    "  -p, --print         Print environment to standard output and exit.",
    "  -s, --pathsep SEP   Path separator.  Default is \":\" on Unix and \";\"",
    "                      on Windows.",
    "  -v  --variable NAME=VALUE",
    "                      Add NAME-VALUE pair to environment.",
    "  -V, --version       Print dlite version number and exit.",
    "  --                  End of options.  The rest is passed to COMMAND.",
    "",
    "By default dlite install paths are pre-pended to existing environment",
    "variables.",
    "",
    NULL
  };
  for (p=msg; *p; p++) printf("%s\n", *p);
}


/*
  If `name` is in the environment, prepend `paths` (followed by
  `pathsep`) to the existing value.  Otherwise, just add `paths` to
  `env`.

  Returns non-zero on error.
*/
int prepend_paths(char **env, const char *name, const char *paths,
                  const char *pathsep)
{
  char **q;
  if (!paths || !*paths) return 0;
  if ((q = get_envitem(env, name))) {
    char *item;
    int n = strlen(name);
    int len = strlen(*q) + strlen(paths) + strlen(pathsep) + 1;
    if (!(item = malloc(len))) return err(1, "allocation failure");
    snprintf(item, len, "%s=%s%s%s", name, paths, pathsep, *q+n+1);
    free(*q);
    *q = item;
  } else {
    set_envvar(env, name, paths);
  }
  return 0;
}



int main(int argc, char *argv[])
{
  int retval = 0;
  char **q;

  /* Command line arguments */
  int with_build=0, with_env=1, with_install=1, print=0;
  char **args=NULL, **env=NULL, **vars=NULL, *pathsep;
  //char *path, *ld_library_path, *pythonpath, *storage_plugins,
  //  *mapping_plugins, *python_storage_plugins, *python_mapping_plugins,
  //  *templates, *storages;

  err_set_prefix("dlite-env");

#ifdef WINDOWS
  pathsep = ";";
#else
  pathsep = ":";
#endif

  /* Parse options and arguments */
  while (1) {
    int longindex = 0;
    struct option longopts[] = {
      {"build",         0, NULL, 'b'},
      {"empty",         0, NULL, 'e'},
      {"help",          0, NULL, 'h'},
      {"no-install",    0, NULL, 'i'},
      {"print",         0, NULL, 'p'},
      {"pathsep",       1, NULL, 's'},
      {"variable",      1, NULL, 'v'},
      {"version",       0, NULL, 'V'},
      {NULL, 0, NULL, 0}
    };
    int c = getopt_long(argc, argv, "behips:v:V", longopts, &longindex);
    if (c == -1) break;
    switch (c) {
    case 'b':  with_build = 1; break;
    case 'e':  with_env = 0; break;
    case 'h':  help(stdout); exit(0);
    case 'i':  with_install = 0; break;
    case 'p':  print = 1; break;
    case 's':  pathsep = optarg; break;
    case 'v':  strlist_add(vars, optarg); break;
    case 'V':  printf("%s\n", dlite_VERSION); exit(0);
    case '?':  exit(1);
    default:   abort();
    }
  }

  /* Create environment */
  if (with_env)  {
    env = get_environment();
  }
  if (with_install) {
    /*
    prepend_paths(env, "PATH", DLITE_PATH, pathsep);
    prepend_paths(env, "LD_LIBRARY_PATH", DLITE_LD_LIBRARY_PATH, pathsep);
    prepend_paths(env, "DLITE_STORAGE_PLUGIN_DIRS",
                  DLITE_STORAGE_PLUGIN_DIRS, pathsep);
    prepend_paths(env, "DLITE_MAPPING_PLUGIN_DIRS",
                  DLITE_MAPPING_PLUGIN_DIRS, pathsep);
    prepend_paths(env, "DLITE_PYTHON_STORAGE_PLUGIN_DIRS",
                  DLITE_PYTHON_STORAGE_PLUGIN_DIRS, pathsep);
    prepend_paths(env, "DLITE_PYTHON_MAPPING_PLUGIN_DIRS",
                  DLITE_PYTHON_MAPPING_PLUGIN_DIRS, pathsep);
    prepend_paths(env, "DLITE_TEMPLATE_DIRS", DLITE_TEMPLATE_DIRS, pathsep);
    prepend_paths(env, "DLITE_STORAGES", DLITE_STORAGES, pathsep);
    */
  }
  if (with_build) {
    prepend_paths(env, "PATH", dlite_PATH, pathsep);
    prepend_paths(env, "LD_LIBRARY_PATH", dlite_LD_LIBRARY_PATH, pathsep);
    prepend_paths(env, "DLITE_STORAGE_PLUGIN_DIRS", dlite_STORAGE_PLUGINS,
                  pathsep);
    prepend_paths(env, "DLITE_MAPPING_PLUGIN_DIRS", dlite_MAPPING_PLUGINS,
                  pathsep);
    prepend_paths(env, "DLITE_PYTHON_STORAGE_PLUGIN_DIRS",
                  dlite_PYTHON_STORAGE_PLUGINS, pathsep);
    prepend_paths(env, "DLITE_PYTHON_MAPPING_PLUGIN_DIRS",
                  dlite_PYTHON_MAPPING_PLUGINS, pathsep);
    prepend_paths(env, "DLITE_TEMPLATE_DIRS", dlite_TEMPLATES, pathsep);
    prepend_paths(env, "DLITE_STORAGES", dlite_STORAGES, pathsep);
  }
  for (q=vars; *q; q++)
    set_envitem(env, *q);

  if (print) {
    /* Print environment */
    for (q=vars; *q; q++)
      printf("%s\n", *q);

  } else  {
    int i, j;
    if (optind >= argc) FAIL("Missing COMMAND argument");

    printf("*** optind=%d, argc=%d\n", optind, argc);

    /* -- create argument list */
    if (!(args = calloc(argc - optind + 1, sizeof(char **))))
      return err(1, "allocation failure");
    for (i=optind, j=0; i<argc; i++)
      if (!(args[j++] = strdup(argv[i]))) FAIL("allocation failure");

    /* -- execute command */
    retval = exec_process(args[0], args, env);
  }

 fail:
  if (env) strlist_free(env);
  if (args) strlist_free(args);
  if (vars) strlist_free(vars);

  return retval;
}
