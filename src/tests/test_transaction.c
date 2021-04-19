#include "config.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "minunit/minunit.h"
#include "dlite.h"
#include "dlite-collection.h"

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s



/***************************************************************
 * Test transaction
 ***************************************************************/

MU_TEST(test_transaction_load)
{

}


MU_TEST(test_transaction_free)
{

}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
#ifdef WITH_JSON
  MU_RUN_TEST(test_transaction_load);     /* setup */

  MU_RUN_TEST(test_transaction_free);     /* tear down */
#endif

}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
