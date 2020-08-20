#include <stdlib.h>
#include <stdio.h>

#include "minunit/minunit.h"

#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-storage-plugins.h"

/* This header should define HOST, DATABASE, USER and PASSWORD */
#include "pgconf.h"


#ifdef PASSWORD
char *options = "database=" DATABASE ";user=" USER ";password=" PASSWORD;
#else
char *options = "database=" DATABASE ";user=" USER;
#endif



MU_TEST(test_load_meta)
{
  DLiteInstance *meta;
  char url[256], *id="http://meta.sintef.no/0.1/Person";
  snprintf(url, sizeof(url), "postgresql://%s?%s#%s", HOST, options, id);
  mu_check((meta = dlite_instance_load_url(url)));

  mu_assert_int_eq(0, dlite_instance_save_url("json:Person2.json?mode=w",
                                              meta));
}

MU_TEST(test_load_inst)
{
  DLiteInstance *inst;
  char url[256], *id="21495524-a02f-5695-82e2-b117addc0b1e";
  snprintf(url, sizeof(url), "postgresql://%s?%s#%s", HOST, options, id);
  mu_check((inst = dlite_instance_load_url(url)));

  mu_assert_int_eq(0, dlite_instance_save_url("json:persons3.json?mode=w",
                                              inst));
}

MU_TEST(test_unload_plugins)
{
  //dlite_storage_plugin_unload_all();
}


/***********************************************************************/


MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_load_inst);
  MU_RUN_TEST(test_load_meta);
  MU_RUN_TEST(test_unload_plugins);
}

int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
