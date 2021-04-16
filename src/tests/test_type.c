#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "minunit/minunit.h"
#include "utils/integers.h"
#include "utils/boolean.h"
#include "utils/err.h"
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
  err_clear();
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
  mu_assert_int_eq(11, size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("string", &type, &size));
  mu_assert_int_eq(dliteStringPtr, type);
  mu_assert_int_eq(sizeof(char *), size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("property", &type, &size));
  mu_assert_int_eq(dliteProperty, type);
  mu_assert_int_eq(sizeof(DLiteProperty), size);

  // ok with comma following the type string
  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("string10,", &type, &size));
  mu_assert_int_eq(dliteFixString, type);
  mu_assert_int_eq(11, size);

  mu_check(dlite_type_set_dtype_and_size("blob5a", &type, &size)); // fails
  mu_assert_int_eq(dliteFixString, type);
  mu_assert_int_eq(11, size);
  err_clear();
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

MU_TEST(test_print)
{
  char buf[128], *ptr=NULL;
  size_t size=0;
  int n;
  double v=3.141592;
  char *p=NULL, s[]="my source string", *q=s;

  mu_assert_int_eq(7, dlite_type_print(buf, sizeof(buf), &v, dliteFloat,
                                       sizeof(double), 0, -2, 0));
  mu_assert_string_eq("3.14159", buf);

  mu_assert_int_eq(4, dlite_type_print(buf, sizeof(buf), &v, dliteFloat,
                                       sizeof(double), 0, 3, 0));
  mu_assert_string_eq("3.14", buf);

  mu_assert_int_eq(6, dlite_type_print(buf, sizeof(buf), &v, dliteFloat,
                                       sizeof(double), 6, 3, 0));
  mu_assert_string_eq("  3.14", buf);

  mu_assert_int_eq(12, dlite_type_print(buf, sizeof(buf), &v, dliteFloat,
                                        sizeof(double), -1, -1, 0));
  mu_assert_string_eq("     3.14159", buf);

  mu_assert_int_eq(18, dlite_type_print(buf, sizeof(buf), &q, dliteStringPtr,
                                        sizeof(char **), -1, -1,
                                        dliteFlagQuoted));
  mu_assert_string_eq("\"my source string\"", buf);

  mu_assert_int_eq(4, dlite_type_print(buf, sizeof(buf), &p, dliteStringPtr,
                                       sizeof(char **), -1, -1, 0));
  mu_assert_string_eq("null", buf);

  n = dlite_type_aprint(&ptr, &size, 0, &q, dliteStringPtr, sizeof(char **),
                        -1, -1, dliteFlagQuoted);
  mu_assert_int_eq(18, n);
  mu_check((int)size > n);
  mu_assert_string_eq("\"my source string\"", ptr);
  free(ptr);
}

MU_TEST(test_scan)
{
  int n;
  unsigned char blob[2];
  bool b;
  int16_t int16;
  uint16_t uint16;
  double float64;
  char buf[10], *s=NULL;
  DLiteDimension dim;
  DLiteProperty prop;
  DLiteRelation rel;

  /* blob */
  n = dlite_type_scan("01ff", blob, dliteBlob, 2, 0);
  mu_assert_int_eq(-1, n);
  err_clear();

  n = dlite_type_scan("\"01fe\"", blob, dliteBlob, 2, 0);
  mu_assert_int_eq(6, n);
  mu_assert_int_eq(1, blob[0]);
  mu_assert_int_eq(254, blob[1]);

  n = dlite_type_scan("01fx", blob, dliteBlob, 2, 0);
  mu_assert_int_eq(-1, n);
  err_clear();

  /* bool */
  n = dlite_type_scan("1", &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(1, n);
  mu_assert_int_eq(1, b);

  n = dlite_type_scan("false", &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(5, n);
  mu_assert_int_eq(0, b);

  n = dlite_type_scan("yes", &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(1, b);

  n = dlite_type_scan(".FALSE.", &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(7, n);
  mu_assert_int_eq(0, b);

  n = dlite_type_scan("1 a", &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(1, n);
  mu_assert_int_eq(1, b);

  n = dlite_type_scan(".", &b, dliteBool, sizeof(bool), 0);
  mu_check(n < 0);
  err_clear();

  /* int */
  n = dlite_type_scan("-35", &int16, dliteInt, sizeof(int16), 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(-35, int16);

  n = dlite_type_scan("0xff", &int16, dliteInt, sizeof(int16), 0);
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(255, int16);

  // hmm, overflow is not reported as error, but silently cast
  // Is this a bug?
  n = dlite_type_scan("1000000  ", &int16, dliteInt, sizeof(int16), 0);
  mu_assert_int_eq(7, n);
  mu_assert_int_eq((int16_t)1000000, int16);

  /* uint */
  n = dlite_type_scan("42", &uint16, dliteUInt, sizeof(uint16), 0);
  mu_assert_int_eq(2, n);
  mu_assert_int_eq(42, uint16);

  n = dlite_type_scan("0xff", &uint16, dliteUInt, sizeof(uint16), 0);
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(255, uint16);

  // hmm, accepts negative integer and cast it to unit16_t
  // Isn't this a bug?
  n = dlite_type_scan("-35", &uint16, dliteUInt, sizeof(uint16), 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq((uint16_t)-35, uint16);

  n = dlite_type_scan("-", &uint16, dliteUInt, sizeof(uint16), 0);
  mu_assert_int_eq(-1, n);
  err_clear();

  /* float */
  n = dlite_type_scan(" 3.14 ", &float64, dliteFloat, sizeof(float64), 0);
  mu_assert_int_eq(5, n);
  mu_assert_double_eq(3.14, float64);

  n = dlite_type_scan(" 2.1e-2 ", &float64, dliteFloat, sizeof(float64), 0);
  mu_assert_int_eq(7, n);
  mu_assert_double_eq(2.1e-2, float64);

  /* fixstring */
  n = dlite_type_scan(" 3.14 ", buf, dliteFixString, sizeof(buf),
                      dliteFlagQuoted);
  mu_assert_int_eq(-1, n);
  err_clear();

  n = dlite_type_scan(" \"3.14\" ", buf, dliteFixString, sizeof(buf),
                      dliteFlagQuoted);
  mu_assert_int_eq(7, n);
  mu_assert_string_eq("3.14", buf);

  n = dlite_type_scan("\"1234567890\"", buf, dliteFixString, sizeof(buf),
                      dliteFlagQuoted);
  mu_assert_int_eq(12, n);
  mu_assert_string_eq("123456789", buf);

  /* string */
  n = dlite_type_scan(" \"3.14\" ", &s, dliteStringPtr, sizeof(char **),
                      dliteFlagQuoted);
  mu_assert_int_eq(7, n);
  mu_assert_string_eq("3.14", s);
  free(s);

  /* dliteDimension */
  memset(&dim, 0, sizeof(DLiteDimension));

  s = "{\"name\": \"nelem\"}";
  n = dlite_type_scan(s, &dim, dliteDimension, sizeof(DLiteDimension), 0);
  mu_assert_int_eq(17, n);
  mu_assert_string_eq("nelem", dim.name);
  mu_assert_string_eq(NULL, dim.description);

  s = "{\"name\": \"N\", \"description\": \"number of items\"}  ";
  n = dlite_type_scan(s, &dim, dliteDimension, sizeof(DLiteDimension), 0);
  mu_assert_int_eq(47, n);
  mu_assert_string_eq("N", dim.name);
  mu_assert_string_eq("number of items", dim.description);

  n = dlite_type_scan("{\"namex\": \"ntokens\"}", &dim, dliteDimension,
                      sizeof(DLiteDimension), 0);
  mu_assert_int_eq(-1, n);
  err_clear();

  s = "{\"name\": \"M\", \"xxx\": \"this is an array\"}";
  n = dlite_type_scan(s, &dim, dliteDimension, sizeof(DLiteDimension), 0);
  mu_assert_int_eq(40, n);
  mu_assert_string_eq("M", dim.name);
  mu_assert_string_eq(NULL, dim.description);

  if (dim.name) free(dim.name);
  if (dim.description) free(dim.description);

  /* dliteProperty */
  memset(&prop, 0, sizeof(DLiteProperty));

  s = "{"
    "\"name\": \"field\", "
    "\"type\": \"blob3\", "
    "\"dims\": [\"N+1\", \"M\"], "
    "\"unit\": \"m\""
    "}";
  n = dlite_type_scan(s, &prop, dliteProperty, sizeof(DLiteProperty), 0);
  mu_assert_int_eq(69, n);
  mu_assert_string_eq("field", prop.name);
  mu_assert_int_eq(dliteBlob, prop.type);
  mu_assert_int_eq(3, prop.size);
  mu_assert_int_eq(2, prop.ndims);
  mu_assert_string_eq("N+1", prop.dims[0]);
  mu_assert_string_eq("M", prop.dims[1]);
  mu_assert_string_eq("m", prop.unit);
  mu_assert_string_eq(NULL, prop.description);

  if (prop.name) free(prop.name);
  for (n=0; n < prop.ndims; n++) free(prop.dims[n]);
  if (prop.dims) free(prop.dims);
  if (prop.unit) free(prop.unit);
  if (prop.description) free(prop.description);

  /* dliteRelation */
  memset(&rel, 0, sizeof(DLiteRelation));

  s = "[\"subject\", \"predicate\", \"object\"]";
  n = dlite_type_scan(s, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(34, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq(NULL,        rel.id);

  if (rel.s) free(rel.s);
  if (rel.p) free(rel.p);
  if (rel.o) free(rel.o);
  if (rel.id) free(rel.id);
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
  size_t sdims[] = {2, 2, 3};
  size_t ddims[] = {2, 2, 3};
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
  MU_RUN_TEST(test_print);
  MU_RUN_TEST(test_scan);
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
