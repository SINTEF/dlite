#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "minunit/minunit.h"
#include "utils/integers.h"
#include "utils/boolean.h"
#include "utils/strutils.h"
#include "utils/err.h"
#include "dlite.h"
#include "dlite-errors.h"


/***************************************************************
 * Test type
 ***************************************************************/

MU_TEST(test_get_dtypename)
{
  mu_assert_string_eq("blob", dlite_type_get_dtypename(dliteBlob));
  mu_assert_string_eq("bool", dlite_type_get_dtypename(dliteBool));
  mu_assert_string_eq("string", dlite_type_get_dtypename(dliteStringPtr));
  mu_assert_string_eq("ref", dlite_type_get_dtypename(dliteRef));
  mu_assert_string_eq("relation", dlite_type_get_dtypename(dliteRelation));
}

MU_TEST(test_get_enum_name)
{
  mu_assert_string_eq("dliteBlob", dlite_type_get_enum_name(dliteBlob));
  mu_assert_string_eq("dliteBool", dlite_type_get_enum_name(dliteBool));
  mu_assert_string_eq("dliteFixString",
                      dlite_type_get_enum_name(dliteFixString));
  mu_assert_string_eq("dliteRef",
                      dlite_type_get_enum_name(dliteRef));
  mu_assert_string_eq("dliteProperty",
                      dlite_type_get_enum_name(dliteProperty));
}

MU_TEST(test_get_dtype)
{
  mu_assert_int_eq(dliteBlob, dlite_type_get_dtype("blob"));
  mu_assert_int_eq(dliteInt, dlite_type_get_dtype("int"));
  mu_assert_int_eq(dliteFloat, dlite_type_get_dtype("float"));
  mu_assert_int_eq(-1, dlite_type_get_dtype("float32"));
  mu_assert_int_eq(dliteRef, dlite_type_get_dtype("ref"));
  mu_assert_int_eq(dliteRef,
                   dlite_type_get_dtype("http://onto-ns.com/meta/0.1/Entity"));
}

MU_TEST(test_set_typename)
{
  char typename[32];
  mu_assert_int_eq(0, dlite_type_set_typename(dliteBlob, 13, typename, 32));
  mu_assert_string_eq("blob13", typename);

  mu_assert_int_eq(0, dlite_type_set_typename(dliteUInt, 8, typename, 32));
  mu_assert_string_eq("uint64", typename);

  mu_assert_int_eq(0, dlite_type_set_typename(dliteRef,
                                              sizeof(DLiteInstance *),
                                              typename, 32));
  mu_assert_string_eq("ref", typename);

  int n;
  n = dlite_type_set_typename(dliteBool, 13, typename, 32);
  mu_assert_int_eq(dliteValueError, n);
  err_clear();
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

  mu_assert_int_eq(17,
                   dlite_type_set_cdecl(dliteRef, sizeof(DLiteInstance *),
                                        "q", 1, decl, 80, 0));
  mu_assert_string_eq("DLiteInstance **q", decl);
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
  mu_check(dlite_is_type("ref"));
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

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("str5", &type, &size));
  mu_assert_int_eq(dliteFixString, type);
  mu_assert_int_eq(6, size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("string", &type, &size));
  mu_assert_int_eq(dliteStringPtr, type);
  mu_assert_int_eq(sizeof(char *), size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("str", &type, &size));
  mu_assert_int_eq(dliteStringPtr, type);
  mu_assert_int_eq(sizeof(char *), size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("ref", &type, &size));
  mu_assert_int_eq(dliteRef, type);
  mu_assert_int_eq(sizeof(DLiteInstance *), size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("http://meta/0.1/Data",
                                                    &type, &size));
  mu_assert_int_eq(dliteRef, type);
  mu_assert_int_eq(sizeof(DLiteInstance *), size);

  // invalid type
  mu_check(dlite_type_set_dtype_and_size("git://meta/0.1/Data", &type, &size));

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("property", &type, &size));
  mu_assert_int_eq(dliteProperty, type);
  mu_assert_int_eq(sizeof(DLiteProperty), size);

  // ok with comma or space following the type string
  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("string8,", &type, &size));
  mu_assert_int_eq(dliteFixString, type);
  mu_assert_int_eq(9, size);

  mu_assert_int_eq(0, dlite_type_set_dtype_and_size("string6 abc", &type, &size));
  mu_assert_int_eq(dliteFixString, type);
  mu_assert_int_eq(7, size);

  mu_check(dlite_type_set_dtype_and_size("blob5a", &type, &size)); // fails
  mu_assert_int_eq(dliteFixString, type);
  mu_assert_int_eq(7, size);  // unchanged
  err_clear();
}

MU_TEST(test_is_allocated)
{
  mu_check(!dlite_type_is_allocated(dliteInt));
  mu_check(!dlite_type_is_allocated(dliteFixString));
  mu_check(dlite_type_is_allocated(dliteStringPtr));
  mu_check(dlite_type_is_allocated(dliteRef));
  mu_check(dlite_type_is_allocated(dliteDimension));
  mu_check(dlite_type_is_allocated(dliteProperty));
  mu_check(dlite_type_is_allocated(dliteRelation));
}

MU_TEST(test_copy)
{
  double dest=0.0, src=3.4;
  char sdst[32]="", ssrc[]="my source string";
  DLiteInstance *idst=NULL, *isrc=(DLiteInstance *)&src;
  mu_check(dlite_type_copy(&dest, &src, dliteFloat, sizeof(src)));
  mu_assert_double_eq(src, dest);

  mu_check(dlite_type_copy(&sdst, &ssrc, dliteFixString, sizeof(ssrc)));
  mu_assert_string_eq(ssrc, sdst);

  mu_check(dlite_type_copy(&idst, &isrc, dliteRef, sizeof(isrc)));
  mu_assert_ptr_eq(isrc, idst);
}

MU_TEST(test_clear)
{
  double v=3.4;
  char s[]="my source string";
  DLiteInstance *i=(DLiteInstance *)dlite_collection_create(NULL);
  mu_check(dlite_type_clear(&v, dliteFloat, sizeof(v)));
  mu_assert_double_eq(0.0, v);

  mu_check(dlite_type_clear(&s, dliteFixString, sizeof(s)));
  mu_assert_string_eq("", s);

  mu_check(dlite_type_clear(&i, dliteRef, sizeof(i)));
  mu_assert_ptr_eq(NULL, i);
}

MU_TEST(test_print)
{
  char buf[128], *ptr=NULL;
  size_t size=0;
  int n;
  double v=3.141592;
  char *p=NULL, s[]="my source string", *q=s;
  DLiteInstance *inst = (DLiteInstance *)dlite_collection_create("myid");
  DLiteDimension d = {"name", "descr"};
  DLiteRelation r1 = {"subject", "predicate", "object", NULL,       NULL};
  DLiteRelation r2 = {"subject", "predicate", "object", "datatype", NULL};

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
                                        sizeof(char *), -1, -1,
                                        dliteFlagQuoted));
  mu_assert_string_eq("\"my source string\"", buf);

  mu_assert_int_eq(4, dlite_type_print(buf, sizeof(buf), &p, dliteStringPtr,
                                       sizeof(char *), -1, -1, 0));
  mu_assert_string_eq("null", buf);

  mu_assert_int_eq(4, dlite_type_print(buf, sizeof(buf), &inst, dliteRef,
                                        sizeof(DLiteInstance *), -1, -1, 0));
  mu_assert_string_eq("myid", buf);

  mu_assert_int_eq(40, dlite_type_print(buf, sizeof(buf), &d, dliteDimension,
                                       sizeof(DLiteDimension *), -1, -1, 0));
  mu_assert_string_eq("{\"name\": \"name\", \"description\": \"descr\"}", buf);


  mu_assert_int_eq(34, dlite_type_print(buf, sizeof(buf), &r1, dliteRelation,
                                       sizeof(DLiteRelation *), -1, -1, 0));
  mu_assert_string_eq("[\"subject\", \"predicate\", \"object\"]", buf);

  mu_assert_int_eq(46, dlite_type_print(buf, sizeof(buf), &r2, dliteRelation,
                                       sizeof(DLiteRelation *), -1, -1, 0));
  mu_assert_string_eq("[\"subject\", \"predicate\", \"object\", \"datatype\"]",
                      buf);

  buf[0] = '\0';
  mu_assert_int_eq(34, dlite_type_print(buf, 0, &r1, dliteRelation,
                                       sizeof(DLiteRelation *), -1, -1, 0));
  mu_assert_int_eq(0, buf[0]);

  n = dlite_type_aprint(&ptr, &size, 0, &q, dliteStringPtr, sizeof(char **),
                        -1, -1, dliteFlagQuoted);
  mu_assert_int_eq(18, n);
  mu_check((int)size > n);
  mu_assert_string_eq("\"my source string\"", ptr);
  free(ptr);
  dlite_instance_decref(inst);
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
  n = dlite_type_scan("01ff", -1, blob, dliteBlob, 2, 0);
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(1, blob[0]);
  mu_assert_int_eq(255, blob[1]);

  n = dlite_type_scan("\"01ff\"", -1, blob, dliteBlob, 2, 0);
  mu_assert_int_eq(-1, n);
  err_clear();

  n = dlite_type_scan("\"01fe\"", -1, blob, dliteBlob, 2, dliteFlagQuoted);
  mu_assert_int_eq(6, n);
  mu_assert_int_eq(1, blob[0]);
  mu_assert_int_eq(254, blob[1]);

  n = dlite_type_scan("01fe", -1, blob, dliteBlob, 2, dliteFlagRaw);
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(1, blob[0]);
  mu_assert_int_eq(254, blob[1]);

  //n = dlite_type_scan("  01fe ", -1, blob, dliteBlob, 2, dliteFlagStrip);
  //mu_assert_int_eq(8, n);
  //mu_assert_int_eq(1, blob[0]);
  //mu_assert_int_eq(254, blob[1]);

  n = dlite_type_scan("01fx", -1, blob, dliteBlob, 2, 0);
  mu_assert_int_eq(-1, n);
  err_clear();

  /* bool */
  n = dlite_type_scan("1", -1, &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(1, n);
  mu_assert_int_eq(1, b);

  n = dlite_type_scan("false", -1, &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(5, n);
  mu_assert_int_eq(0, b);

  n = dlite_type_scan("yes", -1, &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(1, b);

  n = dlite_type_scan(".FALSE.", -1, &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(7, n);
  mu_assert_int_eq(0, b);

  n = dlite_type_scan("1 a", -1, &b, dliteBool, sizeof(bool), 0);
  mu_assert_int_eq(1, n);
  mu_assert_int_eq(1, b);

  n = dlite_type_scan(".", -1, &b, dliteBool, sizeof(bool), 0);
  mu_check(n < 0);
  err_clear();

  /* int */
  n = dlite_type_scan("-35", -1, &int16, dliteInt, sizeof(int16), 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq(-35, int16);

  n = dlite_type_scan("0xff", -1, &int16, dliteInt, sizeof(int16), 0);
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(255, int16);

  // hmm, overflow is not reported as error, but silently cast
  // Is this a bug?
  n = dlite_type_scan("1000000  ", -1, &int16, dliteInt, sizeof(int16), 0);
  mu_assert_int_eq(7, n);
  mu_assert_int_eq((int16_t)1000000, int16);

  /* uint */
  n = dlite_type_scan("42", -1, &uint16, dliteUInt, sizeof(uint16), 0);
  mu_assert_int_eq(2, n);
  mu_assert_int_eq(42, uint16);

  n = dlite_type_scan("0xff", -1, &uint16, dliteUInt, sizeof(uint16), 0);
  mu_assert_int_eq(4, n);
  mu_assert_int_eq(255, uint16);

  // hmm, accepts negative integer and cast it to unit16_t
  // Isn't this a bug?
  n = dlite_type_scan("-35", -1, &uint16, dliteUInt, sizeof(uint16), 0);
  mu_assert_int_eq(3, n);
  mu_assert_int_eq((uint16_t)-35, uint16);

  n = dlite_type_scan("-", -1, &uint16, dliteUInt, sizeof(uint16), 0);
  mu_assert_int_eq(dliteValueError, n);
  err_clear();

  /* float */
  n = dlite_type_scan(" 3.14 ", -1, &float64, dliteFloat, sizeof(float64), 0);
  mu_assert_int_eq(5, n);
  mu_assert_double_eq(3.14, float64);

  n = dlite_type_scan(" 2.1e-2 ", -1, &float64, dliteFloat, sizeof(float64), 0);
  mu_assert_int_eq(7, n);
  mu_assert_double_eq(2.1e-2, float64);

  /* fixstring */
  n = dlite_type_scan(" 3.14 ", -1, buf, dliteFixString, sizeof(buf),
                      dliteFlagQuoted);
  mu_assert_int_eq(-1, n);
  err_clear();

  n = dlite_type_scan(" \"3.14\" ", -1, buf, dliteFixString, sizeof(buf),
                      dliteFlagQuoted);
  mu_assert_int_eq(7, n);
  mu_assert_string_eq("3.14", buf);

  n = dlite_type_scan("\"1234567890\"", -1, buf, dliteFixString, sizeof(buf),
                      dliteFlagQuoted);
  mu_assert_int_eq(12, n);
  mu_assert_string_eq("123456789", buf);

  /* string */
  n = dlite_type_scan(" \"3.14\" ", -1, &s, dliteStringPtr, sizeof(char **),
                      dliteFlagQuoted);
  mu_assert_int_eq(7, n);
  mu_assert_string_eq("3.14", s);
  free(s);

  /* ref */
  DLiteInstance *inst=(DLiteInstance *)dlite_collection_create("http://data.org/collid");
  DLiteInstance *inst2=inst;

  n = dlite_type_scan(" null  ", -1, &inst2,
                      dliteRef, sizeof(DLiteInstance **), 0);
  mu_assert_int_eq(5, n);
  mu_assert_ptr_eq(NULL, inst2);

  n = dlite_type_scan("\"11832981-7097-566e-8e14-51d41b461648\"", -1, &inst2,
                      dliteRef, sizeof(DLiteInstance **), dliteFlagQuoted);
  mu_assert_int_eq(38, n);
  mu_assert_ptr_eq(inst, inst2);

  dlite_instance_decref(inst);
  dlite_instance_decref(inst2);

  /* dliteDimension */
  memset(&dim, 0, sizeof(DLiteDimension));

  s = "{\"name\": \"nelem\"}";
  n = dlite_type_scan(s, -1, &dim, dliteDimension, sizeof(DLiteDimension), 0);
  mu_assert_int_eq(17, n);
  mu_assert_string_eq("nelem", dim.name);
  mu_assert_string_eq(NULL, dim.description);

  s = "{\"name\": \"N\", \"description\": \"number of items\"}  ";
  n = dlite_type_scan(s, -1, &dim, dliteDimension, sizeof(DLiteDimension), 0);
  mu_assert_int_eq(47, n);
  mu_assert_string_eq("N", dim.name);
  mu_assert_string_eq("number of items", dim.description);

  n = dlite_type_scan("{\"namex\": \"ntokens\"}", -1, &dim, dliteDimension,
                      sizeof(DLiteDimension), 0);
  mu_assert_int_eq(dliteValueError, n);
  err_clear();

  s = "{\"name\": \"M\", \"xxx\": \"this is an array\"}";
  n = dlite_type_scan(s, -1, &dim, dliteDimension, sizeof(DLiteDimension), 0);
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
    "\"shape\": [\"N+1\", \"M\"], "
    "\"unit\": \"m\""
    "}";
  n = dlite_type_scan(s, -1, &prop, dliteProperty, sizeof(DLiteProperty), 0);
  mu_assert_int_eq(70, n); //69
  mu_assert_string_eq("field", prop.name);
  mu_assert_int_eq(dliteBlob, prop.type);
  mu_assert_int_eq(3, prop.size);
  mu_assert_int_eq(2, prop.ndims);
  mu_assert_string_eq("N+1", prop.shape[0]);
  mu_assert_string_eq("M", prop.shape[1]);
  mu_assert_string_eq("m", prop.unit);
  mu_assert_string_eq(NULL, prop.description);

  if (prop.name) free(prop.name);
  for (n=0; n < prop.ndims; n++) free(prop.shape[n]);
  if (prop.shape) free(prop.shape);
  if (prop.unit) free(prop.unit);
  if (prop.description) free(prop.description);

  /* dliteRelation */
  memset(&rel, 0, sizeof(DLiteRelation));
  s = "[\"subject\", \"predicate\", \"object\"]";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(34, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq(NULL,        rel.d);
  mu_assert_string_eq(NULL,        rel.id);
  triple_clean(&rel);

  s = "[\"subject\", \"predicate\", \"object\", \"datatype\"]";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(46, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq("datatype",  rel.d);
  mu_assert_string_eq(NULL,        rel.id);
  triple_clean(&rel);

  s = "[\"subject\", \"predicate\", \"object\", \"datatype\", \"id\"]";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(52, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq("datatype",  rel.d);
  mu_assert_string_eq("id",        rel.id);
  triple_clean(&rel);

  s = "[\"subject\", \"predicate\", \"object\", \"\"]";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(38, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq(NULL,        rel.d);
  mu_assert_string_eq(NULL,        rel.id);
  triple_clean(&rel);

  s = "[\"subject\", \"predicate\", \"object\", \"\", \"\"]";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(42, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq(NULL,        rel.d);
  mu_assert_string_eq(NULL,        rel.id);
  triple_clean(&rel);

  s = "[\"subject\", \"predicate\", \"object\", \"\", \"id\"]";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(44, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq(NULL,        rel.d);
  mu_assert_string_eq("id",        rel.id);
  triple_clean(&rel);

  s = "{\"s\": \"subject\", \"p\": \"predicate\", \"o\": \"object\"}";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(49, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq(NULL,        rel.d);
  mu_assert_string_eq(NULL,        rel.id);
  triple_clean(&rel);

  s = "{\"s\": \"subject\", \"p\": \"predicate\", \"o\": \"object\", "
    "\"d\": \"datatype\"}";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(66, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq("datatype",  rel.d);
  mu_assert_string_eq(NULL,        rel.id);
  triple_clean(&rel);

  s = "{\"s\": \"subject\", \"p\": \"predicate\", \"o\": \"object\", "
    "\"d\": \"datatype\", \"id\": \"id\"}";
  n = dlite_type_scan(s, -1, &rel, dliteRelation, sizeof(DLiteRelation), 0);
  mu_assert_int_eq(78, n);
  mu_assert_string_eq("subject",   rel.s);
  mu_assert_string_eq("predicate", rel.p);
  mu_assert_string_eq("object",    rel.o);
  mu_assert_string_eq("datatype",  rel.d);
  mu_assert_string_eq("id",        rel.id);
  triple_clean(&rel);
}


/* Write hex encoded hash to string `s`, which must  be at least 65 bytes. */
static char *gethash(char *s, const void *ptr, DLiteType dtype, size_t size)
{
  sha3_context c;
  int hashsize = 32;
  int bitsize = 8*hashsize;
  const unsigned char *buf;
  sha3_Init(&c, bitsize);
  sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);
  if (dlite_type_update_sha3(&c, ptr, dtype, size)) return NULL;
  buf = sha3_Finalize(&c);
  if (strhex_encode(s, 65, buf, hashsize) < 0) return NULL;
  return s;
}

MU_TEST(test_update_sha3)
{
  char s[65];
  const char *hash;

  int32_t i1 = 42;
  hash = "298c8f103b5a4112a1ab1da335986cfc363f068fcd72c0393382d02a71faa24a";
  mu_assert_string_eq(hash, gethash(s, &i1, dliteInt, sizeof(i1)));

  int32_t i2 = 43;
  hash = "b81c9c72c6322c9aa98c64259488c6a7d27d3638aa329cf272a2c5d1c5637cd6";
  mu_assert_string_eq(hash, gethash(s, &i2, dliteInt, sizeof(i2)));

  bool b1 = 0;
  hash = "bc36789e7a1e281436464229828f817d6612f7b477d66591ff96a9e064bcc98a";
  mu_assert_string_eq(hash, gethash(s, &b1, dliteBool, sizeof(b1)));

  bool b2 = 1;
  hash = "5fe7f977e71dba2ea1a68e21057beebb9be2ac30c6410aa38d4f3fbe41dcffd2";
  mu_assert_string_eq(hash, gethash(s, &b2, dliteBool, sizeof(b2)));

  bool b3 = -1;
  hash = "5fe7f977e71dba2ea1a68e21057beebb9be2ac30c6410aa38d4f3fbe41dcffd2";
  mu_assert_string_eq(hash, gethash(s, &b3, dliteBool, sizeof(b3)));

  char s1[] = "string1";
  hash = "22bceddf404e46d56d0d3770553d3b88745675ea98806dd2adedbad333ff2e9c";
  mu_assert_string_eq(hash, gethash(s, s1, dliteFixString, strlen(s1)));

  char *s2 = s1;
  hash = "22bceddf404e46d56d0d3770553d3b88745675ea98806dd2adedbad333ff2e9c";
  mu_assert_string_eq(hash, gethash(s, &s2, dliteStringPtr, sizeof(s2)));

  DLiteDimension d = {"dimname", "dimdescr"};
  hash = "5aff904f6bed85011648cc1ab16025e3c4900364efdf404005e89bf32b814fc4";
  mu_assert_string_eq(hash, gethash(s, &d, dliteDimension, sizeof(d)));

  d.description = NULL;  // hash depends on description
  hash = "fc28849f70bcc72785d7f8d89ccbd9b1ffb71674bdd4d8dd327c78c7052c3bdb";
  mu_assert_string_eq(hash, gethash(s, &d, dliteDimension, sizeof(d)));

  char *shape[] = {"dim1", "dim2"};
  DLiteProperty p = {"propname", dliteStringPtr, sizeof(char *), NULL, 2, shape,
    "m/s", NULL};
  //hash = "f52af54cb773ec8a7499ae44f3499c58fba40203d870c0c52842d10cbbcdc13a";
  hash = "5d2b98da4531f9a5b519cdebcc5ef181a46a25372d0774869bb98c631b68dce6";
  mu_assert_string_eq(hash, gethash(s, &p, dliteProperty, sizeof(p)));

  p.description = "Some description...";  // hash depends on description
  hash = "ba830a4ffc8cf9472363c892a5346565dd05fa9f7aad03b317609e32295939a7";
  mu_assert_string_eq(hash, gethash(s, &p, dliteProperty, sizeof(p)));

  p.unit = "m";  // hash depends on unit
  hash = "ab378fe9afa100e56e4eed2c564460db18ec8872d5aedfa6d1d0a10a31c8ccff";
  mu_assert_string_eq(hash, gethash(s, &p, dliteProperty, sizeof(p)));
}


MU_TEST(test_get_alignment)
{
  mu_assert_int_eq(1,  dlite_type_get_alignment(dliteUInt, 1));
  mu_assert_int_eq(2,  dlite_type_get_alignment(dliteUInt, 2));
  mu_assert_int_eq(1,  dlite_type_get_alignment(dliteBlob, 3));
  mu_assert_int_eq(1,  dlite_type_get_alignment(dliteBlob, 4));
  mu_assert_int_eq(4,  dlite_type_get_alignment(dliteInt,  4));
  mu_assert_int_eq(1,  dlite_type_get_alignment(dliteFixString, 3));
#if defined(i386) && i386
  mu_assert_int_eq(4,  dlite_type_get_alignment(dliteInt,  8));
  mu_assert_int_eq(4,  dlite_type_get_alignment(dliteRef, 4));
  mu_assert_int_eq(4,  dlite_type_get_alignment(dliteDimension,
                                                sizeof(DLiteDimension)));
#else
  mu_assert_int_eq(8,  dlite_type_get_alignment(dliteInt,  8));
  mu_assert_int_eq(8,  dlite_type_get_alignment(dliteStringPtr, 8));
  mu_assert_int_eq(8,  dlite_type_get_alignment(dliteRef, 8));
  mu_assert_int_eq(8,  dlite_type_get_alignment(dliteDimension,
                                                sizeof(DLiteDimension)));
#endif
#if defined(HAVE_FLOAT80) || defined(HAVE_FLOAT96)|| defined(HAVE_FLOAT128)
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
  mu_assert_int_eq(3, dlite_type_get_member_offset(2, 1, dliteUInt, 1));
  mu_assert_int_eq(3, dlite_type_get_member_offset(2, 1, dliteBlob, 1));
  mu_assert_int_eq(3, dlite_type_get_member_offset(2, 1, dliteBool, 1));
#if defined(i386) && i386
  mu_assert_int_eq(4, dlite_type_get_member_offset(2, 1, dliteInt, 8));
  mu_assert_int_eq(4, dlite_type_get_member_offset(2, 1, dliteStringPtr,
                                                   sizeof(char *)));
  mu_assert_int_eq(4, dlite_type_get_member_offset(2, 1, dliteRelation,
                                                   sizeof(DLiteRelation)));
#else
  mu_assert_int_eq(8, dlite_type_get_member_offset(2, 1, dliteInt, 8));
  mu_assert_int_eq(8, dlite_type_get_member_offset(2, 1, dliteStringPtr,
                                                   sizeof(char *)));
  mu_assert_int_eq(8, dlite_type_get_member_offset(2, 1, dliteRelation,
                                                   sizeof(DLiteRelation)));
#endif
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
  MU_RUN_TEST(test_update_sha3);
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
