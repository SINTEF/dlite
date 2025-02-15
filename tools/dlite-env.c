/* dlite-env.c -- run command in dlite environment */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#include "dlite.h"
#include "utils/compat.h"
#include "utils/compat-src/getopt.h"
#include "utils/err.h"
#include "utils/execprocess.h"
#include "dlite-macros.h"
#include "config-paths.h"

typedef enum {Replace, Append, Prepend} Action;

/* Globals */
FUPlatform platform = fuNative;
const char *dlite_root=NULL, *dlite_pkg_root=NULL;


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
    "  -P  --platform PLATFORM",
    "                      Set environment variables according to this ",
    "                      platform.  PLATFORM should be either \"Unix\" or ",
    "                      \"Windows\".  Defaults to the host platform.",
    "  -v  --variable NAME=VALUE",
    "                      Add NAME-VALUE pair to environment.",
    "  -V, --version       Print dlite version number and exit.",
    "  --                  End of options.  The rest are passed to COMMAND.",
    "",
    "By default dlite install paths are pre-pended to existing environment",
    "variables.",
    "",
    NULL
  };
  for (p=msg; *p; p++) printf("%s\n", *p);
}


/* Update `env` by adding variable `name` and `value` to it.  How it
   is done is determined by `action`, which can be either 'Replace',
   'Append' or 'Prepend'. */
char **add_paths(char **env, const char *name, const char *value, Action action)
{
  FUPaths paths;
  char **q, *v, *s;
  if (!value || !*value) return env;
  fu_paths_init(&paths, NULL);
  fu_paths_set_platform(&paths, platform);

  if (action == Append && (v = get_envvar(env, name)))
    fu_paths_extend_prefix(&paths, dlite_pkg_root, v, NULL);

  fu_paths_extend_prefix(&paths, dlite_pkg_root, value, NULL);

  if (action == Prepend && (v = get_envvar(env, name)))
    fu_paths_extend_prefix(&paths, dlite_pkg_root, v, NULL);

  if (!(s = fu_paths_string(&paths)))
    return err(1, "cannot add %s to environment", name), env;
  if (!(q = set_envvar(env, name, s)))
    return err(1, "cannot set environment variable %s", name), env;
  free(s);
  fu_paths_deinit(&paths);
  return q;
}


/* Set platform */
void set_platform(const char *s)
{
  platform = fu_platform(s);
  if (platform < 0) exit(1);
}


int main(int argc, char *argv[])
{
  int retval = 0;
  char **q;

  /* Command line arguments */
  int with_build=0, with_env=1, with_install=1, print=0;
  char **args=NULL, **env=NULL, **vars=NULL;

  err_set_prefix("dlite-env");

  /* Parse options and arguments */
  while (1) {
    int longindex = 0;
    struct option longopts[] = {
      {"build",         0, NULL, 'b'},
      {"empty",         0, NULL, 'e'},
      {"help",          0, NULL, 'h'},
      {"no-install",    0, NULL, 'i'},
      {"print",         0, NULL, 'p'},
      {"platform",      1, NULL, 'P'},
      {"variable",      1, NULL, 'v'},
      {"version",       0, NULL, 'V'},
      {NULL, 0, NULL, 0}
    };
    int c = getopt_long(argc, argv, "behipP:v:V", longopts, &longindex);
    if (c == -1) break;
    switch (c) {
    case 'b':  with_build=1; with_install=0; break;
    case 'e':  with_env = 0; break;
    case 'h':  help(stdout); exit(0);
    case 'i':  with_install = 0; break;
    case 'p':  print = 1; break;
    case 'P':  set_platform(optarg); break;
    case 'v':  vars = strlist_add(vars, optarg); break;
    case 'V':  printf("%s\n", dlite_VERSION); exit(0);
    case '?':  exit(1);
    default:   abort();
    }
  }

  dlite_root = (with_env) ? dlite_root_get() : DLITE_ROOT;
  dlite_root = (with_env) ? dlite_pkg_root_get() : DLITE_PKG_ROOT;

  /* Create environment */
  if (with_env)  {
    /* -- load current environment */
    env = get_environment();
  }
  if (with_install) {
    /* -- add install paths */
    env = add_paths(env, "DLITE_ROOT", dlite_root, Replace);
    env = add_paths(env, "DLITE_PKG_ROOT", dlite_pkg_root, Replace);
    env = add_paths(env, "PATH", DLITE_RUNTIME_DIR, Prepend);
    env = add_paths(env, "LD_LIBRARY_PATH", DLITE_LIBRARY_DIR, Prepend);
    env = add_paths(env, "PYTHONPATH", DLITE_PYTHONPATH, Prepend);
    env = add_paths(env, "DLITE_STORAGE_PLUGIN_DIRS",
                    DLITE_STORAGE_PLUGIN_DIRS, Prepend);
    env = add_paths(env, "DLITE_MAPPING_PLUGIN_DIRS",
                    DLITE_MAPPING_PLUGIN_DIRS, Prepend);
    env = add_paths(env, "DLITE_PYTHON_STORAGE_PLUGIN_DIRS",
                    DLITE_PYTHON_STORAGE_PLUGIN_DIRS, Prepend);
    env = add_paths(env, "DLITE_PYTHON_MAPPING_PLUGIN_DIRS",
                    DLITE_PYTHON_MAPPING_PLUGIN_DIRS, Prepend);
    env = add_paths(env, "DLITE_TEMPLATE_DIRS",
                    DLITE_TEMPLATE_DIRS, Prepend);
    env = add_paths(env, "DLITE_STORAGES",
                    DLITE_STORAGES, Prepend);
  }
  if (with_build) {
    /* -- add build paths */
    env = add_paths(env, "PATH", dlite_PATH, Prepend);
    env = add_paths(env, "LD_LIBRARY_PATH", dlite_LD_LIBRARY_PATH, Prepend);
    env = add_paths(env, "PYTHONPATH", dlite_PYTHONPATH, Prepend);
    env = set_envvar(env, "DLITE_USE_BUILD_ROOT", "YES");
    env = add_paths(env, "DLITE_STORAGE_PLUGIN_DIRS",
                    dlite_STORAGE_PLUGINS, Replace);
    env = add_paths(env, "DLITE_MAPPING_PLUGIN_DIRS",
                    dlite_MAPPING_PLUGINS, Replace);
    env = add_paths(env, "DLITE_PYTHON_STORAGE_PLUGIN_DIRS",
                    dlite_PYTHON_STORAGE_PLUGINS, Replace);
    env = add_paths(env, "DLITE_PYTHON_MAPPING_PLUGIN_DIRS",
                    dlite_PYTHON_MAPPING_PLUGINS, Replace);
    env = add_paths(env, "DLITE_TEMPLATE_DIRS",
                    dlite_TEMPLATES, Replace);
    env = add_paths(env, "DLITE_STORAGES",
                    dlite_STORAGES, Replace);
  }
  if (vars) {
    /* -- add additional variables from command line */
    for (q=vars; *q; q++)
      env = set_envitem(env, *q);
  }

  if (print) {
    /* Print environment */
    for (q=env; *q; q++)
      printf("%s\n", *q);

  } else  {
    /* Run command */
    int i, j;
    if (optind >= argc) FAIL("Missing COMMAND argument");

    /* -- create argument list */
    if (!(args = calloc(argc - optind + 1, sizeof(char **))))
      return err(dliteMemoryError, "allocation failure");
    for (i=optind, j=0; i<argc; i++)
      if (!(args[j++] = strdup(argv[i]))) FAILCODE(dliteMemoryError, "allocation failure");

    /* -- execute command */
    retval = exec_process(args[0], args, env);
  }

 fail:
  if (env) strlist_free(env);
  if (args) strlist_free(args);
  if (vars) strlist_free(vars);

  return retval;
}
