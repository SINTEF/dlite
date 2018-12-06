#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fileutils.h"
#include "test_macros.h"

#include "minunit/minunit.h"


fu_dir *dir;

MU_TEST(test_fu_opendir)
{
  char *path = STRINGIFY(TESTDIR);
  printf("\n*** path='%s'\n", path);

  dir = fu_opendir(path);
  mu_check(dir);
}


MU_TEST(test_fu_getfile)
{
  const char *fname;
  int found_self=0;
  int found_xyz=0;

  printf("\n");
  fname = fu_nextfile(dir);
  printf("***    %s\n", fname);
  while (fname) {
    printf("***    %s\n", fname);
    if (strcmp(fname, "test_fileutils.c") == 0) found_self=1;
    if (strcmp(fname, "xyz") == 0) found_xyz=1;
    fname = fu_nextfile(dir);
  }

  mu_assert_int_eq(1, found_self);
  mu_assert_int_eq(0, found_xyz);
}


MU_TEST(test_fu_closedir)
{
  mu_assert_int_eq(0, fu_closedir(dir));
}

/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_fu_opendir);       /* setup */
  MU_RUN_TEST(test_fu_getfile);
  MU_RUN_TEST(test_fu_closedir);      /* tear down */
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
