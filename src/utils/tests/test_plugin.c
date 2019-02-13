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
  char *path = STRINGIFY(LIBDIR);
  mu_check((info = plugin_info_create("TestPlugin", "get_testapi", NULL)));
            //"TEST_PLUGIN_PATH")));
  mu_assert_int_eq(0, plugin_path_append(info, path));
}


MU_TEST(test_get_api)
{
  const TestAPI *api;

  mu_check((api = plugin_get_api(info, "testapi")));
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
  MU_RUN_TEST(test_unload);
  MU_RUN_TEST(test_info_free);         /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
