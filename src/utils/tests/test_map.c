#include <stdlib.h>
#include <string.h>

#include "map.h"

#include "minunit/minunit.h"


typedef map_t(unsigned int) map_uint_t;


MU_TEST(test_map)
{
  map_uint_t m;
  unsigned int *p;
  const char *key;
  map_iter_t iter;

  map_init(&m);
  map_set(&m, "testkey", 123);
  p = map_get(&m, "testkey");
  mu_assert_int_eq(123, *p);

  iter = map_iter(&m);
  while ((key = map_next(&m, &iter))) {
    printf("%s -> %u\n", key, *map_get(&m, key));
    /* Note - calling map_remove() while iterating is not allowed! */
    //map_remove(&m, key);
  }

  map_deinit(&m);
}


MU_TEST(test_map2)
{
  map_str_t m;

  map_init(&m);
  char **str = map_get(&m, "no-such-key");
  mu_check(str == NULL);

  map_deinit(&m);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_map);
  MU_RUN_TEST(test_map2);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
