#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "minunit/minunit.h"
#include "utils/integers.h"
#include "utils/boolean.h"
#include "dlite.h"


/***************************************************************
 * Test type
 ***************************************************************/

MU_TEST(test_get_dtypename)
{
  mu_assert_string_eq("blob", dlite_type_get_dtypename(dliteBlob));
  mu_assert_string_eq("bool", dlite_type_get_dtypename(dliteBool));
  mu_assert_string_eq("string", dlite_type_get_dtypename(dliteStringPtr));
  mu_assert_string_eq("relation", dlite_type_get_dtypename(dliteRelation));
}

MU_TEST(test_get_enum_name)
{
  mu_assert_string_eq("dliteBlob", dlite_type_get_enum_name(dliteBlob));
  mu_assert_string_eq("dliteBool", dlite_type_get_enum_name(dliteBool));
  mu_assert_string_eq("dliteFixString",
                      dlite_type_get_enum_name(dliteFixString));
  mu_assert_string_eq("dliteProperty",
                      dlite_type_get_enum_name(dliteProperty));
}

MU_TEST(test_get_dtype)
{
  mu_assert_int_eq(dliteBlob, dlite_type_get_dtype("blob"));
  mu_assert_int_eq(dliteInt, dlite_type_get_dtype("int"));
  mu_assert_int_eq(dliteFloat, dlite_type_get_dtype("float"));
  mu_assert_int_eq(-1, dlite_type_get_dtype("float32"));
}

MU_TEST(test_set_typename)
{
  char typename[32];
  mu_assert_int_eq(0, dlite_type_set_typename(dliteBlob, 13, typename, 32));
  mu_assert_string_eq("blob13", typename);

  mu_assert_int_eq(0, dlite_type_set_typename(dliteUInt, 8, typename, 32));
  mu_assert_string_eq("uint64", typename);
}

MU_TEST(test_set_cdecl)
{
  char decl[80];
  mu_assert_int_eq(13,
                   dlite_type_set_cdecl(dliteBlob, 13, "x", 0, decl, 80, 0));
  mu_assert_string_eq("uint8_t x[13]", decl);

  mu_assert_int_eq(10,
                   dlite_type_set_cdecl(dliteInt, 4, "n", 1, decl, 80, 0));
  mu_assert_string_eq("int32_t *n", decl);

  mu_assert_int_eq(6,
                   dlite_type_set_cdecl(dliteInt, 4, "n", 1, decl, 80, 1));
  mu_assert_string_eq("int *n", decl);
}

MU_TEST(test_is_type)
{
  mu_check(dlite_is_type("float32"));
  mu_check(!dlite_is_type("float32_t"));
  mu_check(dlite_is_type("double"));
  mu_check(dlite_is_type("longdouble"));
  mu_check(dlite_is_type("blob42"));
  mu_check(dlite_is_type("string60"));
}

MU_TEST(test_set_dtype_and_size)
{
  DLiteType type;
  size_t size;
  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("float32", &type, &size));
  mu_assert_int_eq(dliteFloat, type);
  mu_assert_int_eq(4, size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("string10", &type, &size));
  mu_assert_int_eq(dliteFixString, type);
  mu_assert_int_eq(10, size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("string", &type, &size));
  mu_assert_int_eq(dliteStringPtr, type);
  mu_assert_int_eq(sizeof(char *), size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("property", &type, &size));
  mu_assert_int_eq(dliteProperty, type);
  mu_assert_int_eq(sizeof(DLiteProperty), size);
}

MU_TEST(test_is_allocated)
{
  mu_check(!dlite_type_is_allocated(dliteInt));
  mu_check(!dlite_type_is_allocated(dliteFixString));
  mu_check(dlite_type_is_allocated(dliteStringPtr));
  mu_check(dlite_type_is_allocated(dliteDimension));
  mu_check(dlite_type_is_allocated(dliteProperty));
  mu_check(dlite_type_is_allocated(dliteRelation));
}

MU_TEST(test_copy)
{
  double dest, src=3.4;
  char sdst[32], ssrc[]="my source string";
  mu_check(dlite_type_copy(&dest, &src, dliteFloat, sizeof(double)));
  mu_assert_double_eq(src, dest);

  mu_check(dlite_type_copy(&sdst, &ssrc, dliteFixString, sizeof(ssrc)));
  mu_assert_string_eq(ssrc, sdst);
}

MU_TEST(test_clear)
{
  double v=3.4;
  char s[]="my source string";
  mu_check(dlite_type_clear(&v, dliteFloat, sizeof(double)));
  mu_assert_double_eq(0.0, v);

  mu_check(dlite_type_clear(&s, dliteFixString, sizeof(s)));
  mu_assert_string_eq("", s);
}

MU_TEST(test_snprintf)
{
  char buf[128];
  double v=3.141592;
  char *p=NULL, s[]="my source string", *q=s;

  mu_assert_int_eq(7, dlite_type_snprintf(&v, dliteFloat, sizeof(double),
                                          0, -2, buf, sizeof(buf)));
  mu_assert_string_eq("3.14159", buf);

  mu_assert_int_eq(4, dlite_type_snprintf(&v, dliteFloat, sizeof(double),
                                          0, 3, buf, sizeof(buf)));
  mu_assert_string_eq("3.14", buf);

  mu_assert_int_eq(6, dlite_type_snprintf(&v, dliteFloat, sizeof(double),
                                          6, 3, buf, sizeof(buf)));
  mu_assert_string_eq("  3.14", buf);

  mu_assert_int_eq(12, dlite_type_snprintf(&v, dliteFloat, sizeof(double),
                                           -1, -1, buf, sizeof(buf)));
  mu_assert_string_eq("     3.14159", buf);

  mu_assert_int_eq(16, dlite_type_snprintf(&q, dliteStringPtr, sizeof(char *),
                                           -1, -1, buf, sizeof(buf)));
  mu_assert_string_eq("my source string", buf);

  mu_assert_int_eq(6, dlite_type_snprintf(&p, dliteStringPtr, sizeof(char *),
                                           -1, -1, buf, sizeof(buf)));
  mu_assert_string_eq("(null)", buf);
}

MU_TEST(test_get_alignment)
{
  mu_assert_int_eq(1,  dlite_type_get_alignment(dliteUInt, 1));
  mu_assert_int_eq(2,  dlite_type_get_alignment(dliteUInt, 2));
  mu_assert_int_eq(1,  dlite_type_get_alignment(dliteBlob, 3));
  mu_assert_int_eq(1,  dlite_type_get_alignment(dliteBlob, 4));
  mu_assert_int_eq(4,  dlite_type_get_alignment(dliteInt,  4));
  mu_assert_int_eq(8,  dlite_type_get_alignment(dliteInt,  8));
  mu_assert_int_eq(1,  dlite_type_get_alignment(dliteFixString, 3));
  mu_assert_int_eq(8,  dlite_type_get_alignment(dliteStringPtr, 8));
  mu_assert_int_eq(8,  dlite_type_get_alignment(dliteDimension,
                                                sizeof(DLiteDimension)));
#if defined(HAVE_FLOAT80) || defined(HAVE_FLOAT128)
  mu_assert_int_eq(16, dlite_type_get_alignment(dliteFloat, 16));
#endif

}

MU_TEST(test_padding_at)
{
  mu_assert_int_eq(0, dlite_type_padding_at(dliteBlob, 3, 0));
  mu_assert_int_eq(0, dlite_type_padding_at(dliteBlob, 3, 6));
  mu_assert_int_eq(0, dlite_type_padding_at(dliteUInt, 1, 2));
  mu_assert_int_eq(2, dlite_type_padding_at(dliteUInt, 4, 2));
}

MU_TEST(test_get_member_offset)
{
  mu_assert_int_eq(4, dlite_type_get_member_offset(2, 2, dliteInt, 2));
  mu_assert_int_eq(4, dlite_type_get_member_offset(2, 1, dliteInt, 2));
  mu_assert_int_eq(4, dlite_type_get_member_offset(2, 1, dliteInt, 4));
  mu_assert_int_eq(8, dlite_type_get_member_offset(2, 1, dliteInt, 8));
  mu_assert_int_eq(3, dlite_type_get_member_offset(2, 1, dliteUInt, 1));
  mu_assert_int_eq(3, dlite_type_get_member_offset(2, 1, dliteBlob, 1));
  mu_assert_int_eq(3, dlite_type_get_member_offset(2, 1, dliteBool, 1));
  mu_assert_int_eq(8, dlite_type_get_member_offset(2, 1, dliteStringPtr,
                                                   sizeof(char *)));
  mu_assert_int_eq(8, dlite_type_get_member_offset(2, 1, dliteRelation,
                                                   sizeof(DLiteRelation)));
}

MU_TEST(test_copy_cast)
{
  double v=3.14, d1;
  float d2;
  int d3;
  char d4[10], *d5=NULL;
  mu_assert_int_eq(0, dlite_type_copy_cast(&d1, dliteFloat, sizeof(double),
                                           &v, dliteFloat, sizeof(double)));
  mu_assert_double_eq(3.14, d1);

  mu_assert_int_eq(0, dlite_type_copy_cast(&d2, dliteFloat, sizeof(float),
                                           &v, dliteFloat, sizeof(double)));
  mu_assert_double_eq(3.14, ((int)(d2*1e5 + 0.5))/1e5);

  mu_assert_int_eq(0, dlite_type_copy_cast(&d3, dliteInt, sizeof(int),
                                           &v, dliteFloat, sizeof(double)));
  mu_assert_int_eq(3, d3);

  mu_assert_int_eq(0, dlite_type_copy_cast(&d4, dliteFixString, sizeof(d4),
                                           &v, dliteFloat, sizeof(double)));
  mu_assert_string_eq("3.14", d4);

  mu_assert_int_eq(0, dlite_type_copy_cast(&d4, dliteBlob, sizeof(d4),
                                           &v, dliteFloat, sizeof(double)));
  mu_assert_double_eq(3.14, *(double*)d4);

  mu_assert_int_eq(0, dlite_type_copy_cast(&d5, dliteStringPtr, sizeof(char *),
                                           &v, dliteFloat, sizeof(double)));
  mu_assert_string_eq("3.14", d5);
  free(d5);

}

MU_TEST(test_type_ndcast)
{
  int s[] = {0, 1, 2,
             3, 4, 5,

             6, 7, 8,
             9, 10, 11};
  int sdims[] = {2, 2, 3};
  int ddims[] = {2, 2, 3};
  int sstrides[] = {24, 12, 4};
  int dstrides[] = {48, 24, 8};
  uint64_t d[12];
  mu_assert_int_eq(0, dlite_type_ndcast(3,
                                        d, dliteUInt, 8,
                                        ddims, dstrides,
                                        s, dliteInt, sizeof(int),
                                        sdims, sstrides,
                                        NULL));
  mu_assert_int_eq(0, d[0]);
  mu_assert_int_eq(1, d[1]);
  mu_assert_int_eq(2, d[2]);
  mu_assert_int_eq(3, d[3]);
  mu_assert_int_eq(4, d[4]);
  mu_assert_int_eq(11, d[11]);


  mu_assert_int_eq(0, dlite_type_ndcast(3,
                                        d, dliteUInt, 8,
                                        ddims, NULL,
                                        s, dliteInt, sizeof(int),
                                        sdims, NULL,
                                        NULL));
  mu_assert_int_eq(0, d[0]);
  mu_assert_int_eq(1, d[1]);
  mu_assert_int_eq(2, d[2]);
  mu_assert_int_eq(3, d[3]);
  mu_assert_int_eq(4, d[4]);
  mu_assert_int_eq(11, d[11]);


  ddims[0] = 3;
  ddims[1] = 2;
  ddims[2] = 2;
  dstrides[0] = 8;
  dstrides[1] = 24;
  dstrides[2] = 48;
  mu_assert_int_eq(0, dlite_type_ndcast(3,
                                        d, dliteUInt, 8,
                                        ddims, dstrides,
                                        s, dliteInt, sizeof(int),
                                        sdims, sstrides,
                                        NULL));
  mu_assert_int_eq(0, d[0]);
  mu_assert_int_eq(1, d[6]);
  mu_assert_int_eq(2, d[3]);
  mu_assert_int_eq(3, d[9]);
  mu_assert_int_eq(4, d[1]);
  mu_assert_int_eq(5, d[7]);
  mu_assert_int_eq(6, d[4]);
  mu_assert_int_eq(7, d[10]);
  mu_assert_int_eq(8, d[2]);
  mu_assert_int_eq(9, d[8]);
  mu_assert_int_eq(10, d[5]);
  mu_assert_int_eq(11, d[11]);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_get_dtypename);
  MU_RUN_TEST(test_get_enum_name);
  MU_RUN_TEST(test_get_dtype);
  MU_RUN_TEST(test_set_typename);
  MU_RUN_TEST(test_set_cdecl);
  MU_RUN_TEST(test_is_type);
  MU_RUN_TEST(test_set_dtype_and_size);
  MU_RUN_TEST(test_is_allocated);
  MU_RUN_TEST(test_copy);
  MU_RUN_TEST(test_clear);
  MU_RUN_TEST(test_snprintf);
  MU_RUN_TEST(test_get_alignment);
  MU_RUN_TEST(test_padding_at);
  MU_RUN_TEST(test_get_member_offset);
  MU_RUN_TEST(test_copy_cast);
  MU_RUN_TEST(test_type_ndcast);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
