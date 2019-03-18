#include <stdio.h>

#include "minunit/minunit.h"

#include "utils/fileutils.h"

#include "dlite.h"
#include "dlite-mapping.h"
#include "dlite-mapping-plugins.h"




MU_TEST(test_mapping_load)
{
  dlite_mapping_plugin_path_insert(0, );

  
  dlite_mapping_plugin_path_append
  size_t i;
  int ndims=2, dims[]={3,4};
  for (i=0; i<sizeof(data); i++) data[i] = i;
  mu_check((arr = dlite_array_create(data, dliteInt, sizeof(int),
				     ndims, dims)));
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_mapping_load);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
