#include "plugin.h"
#include "test_plugin.h"


static int fun1(int a, int b)
{
  return a + b;
}

static double fun2(double a)
{
  return 2.0 * a;
}


static TestAPI testapi = {
  "testapi",
  fun1,
  fun2
};


DSL_EXPORT const TestAPI *get_testapi(int *iter)
{
  (void)(iter);  /* unused */
  return &testapi;
}
