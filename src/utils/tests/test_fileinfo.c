#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fileinfo.h"
#include "test_macros.h"

#include "minunit/minunit.h"


MU_TEST(test_exists)
{
  mu_check( fileinfo_exists("."));
  mu_check( fileinfo_exists("../.."));
  mu_check(!fileinfo_exists("..."));
  mu_check(!fileinfo_exists(""));
  mu_check( fileinfo_exists("cmake_install.cmake"));
}

MU_TEST(test_isdir)
{
  mu_check( fileinfo_isdir("."));
  mu_check( fileinfo_isdir("../.."));
  mu_check(!fileinfo_isdir("..."));
  mu_check(!fileinfo_isdir(""));
  mu_check(!fileinfo_isdir("cmake_install.cmake"));
}

MU_TEST(test_isnormal)
{
  mu_check(!fileinfo_isnormal("."));
  mu_check(!fileinfo_isnormal("../.."));
  mu_check(!fileinfo_isnormal("..."));
  mu_check(!fileinfo_isnormal(""));
  mu_check( fileinfo_isnormal("cmake_install.cmake"));
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_exists);
  MU_RUN_TEST(test_isdir);
  MU_RUN_TEST(test_isnormal);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
