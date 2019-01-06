#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tmpfileplus.h"
#include "test_macros.h"

#include "minunit/minunit.h"


MU_TEST(test_tempfileplus)
{
  char *tmpdir = STRINGIFY(LIBDIR) "/tmp";
  char *tmpfile=NULL;
  FILE *fp = tmpfileplus(tmpdir, NULL, &tmpfile, 0);
  printf("\n*** tmpdir='%s'\n", tmpdir);
  printf("\n*** tmpfile='%s'\n", tmpfile);

  mu_check(fp);
  mu_check(tmpfile);

  fclose(fp);
  free(tmpfile);
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_tempfileplus);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
