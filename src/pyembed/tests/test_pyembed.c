#include <Python.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-pyembed.h"


typedef DLiteInstance *(*fun_t)(const char *id);


MU_TEST(test_load_modules)
{
  int i;
  FUPaths paths;
  PyObject *plugins;

  fu_paths_init(&paths, "DLITE_PYTHON_MAPPING_PLUGINS");
  fu_paths_insert(&paths, STRINGIFY(TESTDIR), 0);

  plugins = dlite_pyembed_load_plugins(&paths, "DLiteMappingBase");
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



MU_TEST(test_get_address)
{
  const char *id = "http://meta.sintef.no/0.3/EntitySchema";
  void *addr;
  fun_t fun;
  DLiteInstance *inst;

  addr = dlite_pyembed_get_address("dlite_instance_get");
  mu_check(addr);

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
  fun = addr;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

  inst = fun(id);
  mu_check(inst);
  //printf("\nInstance uri: %s\n", inst->uri);
  mu_assert_string_eq(id, inst->uri);

  dlite_instance_decref(inst);
}


MU_TEST(test_get_instance)
{
  const char *id = "http://meta.sintef.no/0.3/EntitySchema";
  PyObject *instance = dlite_pyembed_from_instance(id);
  mu_check(instance);
  printf("\nPython instance: ");
  PyObject_Print(instance, stdout, 0);
  printf("\n");
  Py_XDECREF(instance);
}


MU_TEST(test_finalize)
{
  mu_assert_int_eq(0, dlite_pyembed_finalise());
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_load_modules);
  MU_RUN_TEST(test_get_address);
  MU_RUN_TEST(test_get_instance);
  MU_RUN_TEST(test_finalize);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
