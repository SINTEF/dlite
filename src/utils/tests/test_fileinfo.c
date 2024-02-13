#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fileinfo.h"
#include "test_macros.h"

#include "minunit/minunit.h"


char *abs_dir = STRINGIFY(TESTDIR);
char *abs_file = STRINGIFY(TESTDIR) "/test_fileinfo.c";


MU_TEST(test_exists)
{
  printf("\n");
  printf("abs_dir:  %s\n", abs_dir);
  printf("abs_file: %s\n", abs_file);

  mu_check( fileinfo_exists("."));
  mu_check( fileinfo_exists("../.."));
#ifndef WINDOWS
  /* Windows thinks this is an existing directory - this is a bug!
     Since it is a rather uncommon case, we ignore it for now... */
  mu_check(!fileinfo_exists("..."));
#endif
  mu_check(!fileinfo_exists(""));
  mu_check( fileinfo_exists(abs_dir));
  mu_check( fileinfo_exists(abs_file));
}

MU_TEST(test_isdir)
{
  mu_check( fileinfo_isdir("."));
  mu_check( fileinfo_isdir("../.."));
#ifndef WINDOWS
  /* Windows thinks this is an existing directory - this is a bug!
     Since it is a rather uncommon case, we ignore it for now... */
  mu_check(!fileinfo_isdir("..."));
#endif
  mu_check(!fileinfo_isdir(""));
  mu_check( fileinfo_isdir(abs_dir));
  mu_check(!fileinfo_isdir(abs_file));
}

MU_TEST(test_isnormal)
{
  mu_check(!fileinfo_isnormal("."));
  mu_check(!fileinfo_isnormal("../.."));
  mu_check(!fileinfo_isnormal("..."));
  mu_check(!fileinfo_isnormal(""));
  mu_check(!fileinfo_isnormal(abs_dir));
  mu_check( fileinfo_isnormal(abs_file));
}

MU_TEST(test_isreadable)
{
  mu_check(!fileinfo_isreadable("."));
  mu_check(!fileinfo_isreadable("../.."));
  mu_check(!fileinfo_isreadable("..."));
  mu_check(!fileinfo_isreadable(""));
  mu_check(!fileinfo_isreadable(abs_dir));
  mu_check( fileinfo_isreadable(abs_file));
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_exists);
  MU_RUN_TEST(test_isdir);
  MU_RUN_TEST(test_isnormal);
  MU_RUN_TEST(test_isreadable);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
