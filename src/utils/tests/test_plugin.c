#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "err.h"
#include "plugin.h"
#include "test_plugin.h"
#include "test_macros.h"

#include "minunit/minunit.h"


PluginInfo *info=NULL;


MU_TEST(test_info_create)
{
  char *path = STRINGIFY(BINDIR);
  mu_check((info = plugin_info_create("TestPlugin", "get_testapi", NULL, NULL)));
            //"TEST_PLUGIN_PATH")));
  mu_assert_int_eq(0, plugin_path_append(info, path));
}


MU_TEST(test_get_api)
{
  const TestAPI *api;
  mu_check((api = (const TestAPI *)plugin_get_api(info, "testapi", -2)));
  mu_assert_string_eq("testapi", api->name);
  mu_assert_int_eq(4, api->fun1(1, 3));
  mu_assert_double_eq(6.28, api->fun2(3.14));
}


MU_TEST(test_iter)
{
  int n=0;
  const void *p;
  PluginIter iter;
  const TestAPI *api;
  plugin_api_iter_init(&iter, info);
  while ((p = plugin_api_iter_next(&iter))) {
    api = (const TestAPI *)p;
    n++;
  }
  mu_assert_int_eq(1, n);
  mu_assert_string_eq("testapi", api->name);
}


MU_TEST(test_unload)
{
 ErrTry:
  mu_check(plugin_unload(info, "xxx"));
  break;
 ErrCatch(1):
  break;
 ErrEnd;

  mu_assert_int_eq(0, plugin_unload(info, "testapi"));
}

MU_TEST(test_info_free)
{
  plugin_info_free(info);
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_info_create);       /* setup */
  MU_RUN_TEST(test_get_api);
  MU_RUN_TEST(test_iter);
  MU_RUN_TEST(test_unload);
  MU_RUN_TEST(test_info_free);         /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
