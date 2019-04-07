#include <Python.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-pyembed.h"



MU_TEST(test_load_modules)
{
  int i;
  FUPaths paths;
  char *classname = "MyBase";
  PyObject *plugins;

  fu_paths_init(&paths, "DLITE_PYTHON_MAPPINGS");
  fu_paths_insert(&paths, STRINGIFY(TESTDIR), 0);

  plugins = dlite_pyembed_load_plugins(&paths, classname);
  mu_check(plugins);
  mu_check(PyList_Check(plugins));

  printf("\nLoaded plugins:\n");
  for (i=0; i < (int)PyList_Size(plugins); i++) {
    PyObject *plugin = PyList_GetItem(plugins, i);
    PyObject *name = PyObject_GetAttrString(plugin, "name");
    printf("  - %s\n", (char *)PyUnicode_DATA(name));
    //PyObject_Print(name, stdout, 1);
  }

  fu_paths_deinit(&paths);
  Py_DECREF(plugins);
}



MU_TEST(test_finalize)
{
  mu_assert_int_eq(0, dlite_pyembed_finalise());
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_load_modules);
  MU_RUN_TEST(test_finalize);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
