#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "bson.h"

#include "minunit/minunit.h"


/* Declarations not in bson.h */
int bson_datasize(BsonType type);
int bson_elementsize(BsonType type, const char *ename, int size);


MU_TEST(test_elementsize)
{
  mu_assert_int_eq(11, bson_elementsize(bsonDouble, "x", 8));
  mu_assert_int_eq(15, bson_elementsize(bsonDouble, "value", 8));
  mu_assert_int_eq(15, bson_elementsize(bsonDouble, "xalue", -1));
  mu_assert_int_eq(bsonValueError, bson_elementsize(bsonDouble, "value", 0));

  mu_assert_int_eq(16, bson_elementsize(bsonString, "x", 8));
  mu_assert_int_eq(11, bson_elementsize(bsonDocument, "x", 8));
  mu_assert_int_eq(11, bson_elementsize(bsonArray, "x", 8));
  mu_assert_int_eq(16, bson_elementsize(bsonBinary, "x", 8));
  mu_assert_int_eq(4, bson_elementsize(bsonBool, "x", 1));
  mu_assert_int_eq(3, bson_elementsize(bsonNull, "x", 0));
  mu_assert_int_eq(7, bson_elementsize(bsonInt32, "x", 4));
#ifdef HAVE_INT64
  mu_assert_int_eq(11, bson_elementsize(bsonInt64, "x", 8));
#endif
#ifdef HAVE_FLOAT128
  mu_assert_int_eq(19, bson_elementsize(bsonDecimal128, "x", 16));
#endif
}


/* Reproduce example1 on https://bsonspec.org/faq.html */
MU_TEST(test_example1)
{
  unsigned char doc[1024], expected[] =
    "\x16\x00\x00\x00"
    "\x02"
    "hello\x00"
    "\x06\x00\x00\x00world\x00";
  int n, m, bufsize=sizeof(doc);
  n = bson_init_document(doc, bufsize);
  m = bson_append(doc, bufsize-n, bsonString, "hello", 5, "world");
  mu_check(m > 0);
  n += m;
  mu_assert_int_eq(0x16, bson_docsize(doc));
  mu_assert_int_eq(0x16, n);
  mu_assert_string_eq((char *)expected, (char *)doc);

  char *ename, *data;
  int datasize;
  unsigned char *endptr=NULL;
  bson_parse(doc, &ename, (void **)&data, &datasize, &endptr);
  mu_assert_string_eq("hello", ename);
  mu_assert_string_eq("world", data);
  mu_assert_int_eq(5, datasize);
  mu_check(*endptr == '\x00');

  mu_assert_int_eq(0x16, bson_docsize(doc));
  mu_assert_int_eq(1, bson_nelements(doc));
}


/* Reproduce example2 on https://bsonspec.org/faq.html */
MU_TEST(test_example2)
{
  unsigned char doc[1024], arr[128], expected[] =
    "\x31\x00\x00\x00"
    "\x04BSON\x00"
    "\x26\x00\x00\x00"
    "\x02\x30\x00\x08\x00\x00\x00awesome\x00"
    "\x01\x31\x00\x33\x33\x33\x33\x33\x33\x14\x40"
    "\x10\x32\x00\xc2\x07\x00\x00"
    "\x00";
  int n, n2, m, bufsize=sizeof(doc), arrsize=sizeof(arr);

  memset(doc, 0, bufsize);
  memset(arr, 0, arrsize);

  n2 = bson_init_document(arr, arrsize);
  m = bson_append(arr, arrsize-n2, bsonString, "0", 7, "awesome");
  mu_check(m > 0);
  n2 += m;
  double v1 = 5.05;
  m = bson_append(arr, arrsize-n2, bsonDouble, "1", -1, &v1);
  mu_check(m > 0);
  n2 += m;
  int32_t v2 = 1986;
  m = bson_append(arr, arrsize-n2, bsonInt32, "2", -1, &v2);
  mu_check(m > 0);
  n2 += m;
  mu_assert_int_eq(0x26, bson_docsize(arr));
  mu_assert_int_eq(0x26, n2);

  n = bson_init_document(doc, bufsize);
  m = bson_append(doc, bufsize-n, bsonArray, "BSON", n2, arr);
  mu_check(m > 0);
  n += m;
  mu_assert_int_eq(0x31, bson_docsize(doc));
  mu_assert_int_eq(0x31, n);
  mu_assert_string_eq((char *)expected, (char *)doc);

  char *ename;
  int type, datasize;
  void *data;
  unsigned char *arrdata, *endptr=NULL, *endptr2=NULL;
  type = bson_parse(doc, &ename, (void **)&arrdata, &datasize, &endptr);
  mu_assert_int_eq(bsonArray, type);
  mu_assert_string_eq("BSON", ename);
  mu_assert_int_eq(0x26, datasize);
  type = bson_parse(arrdata, &ename, &data, &datasize, &endptr2);
  mu_assert_int_eq(bsonString, type);
  mu_assert_string_eq("0", ename);
  mu_assert_string_eq("awesome", data);
  mu_assert_int_eq(7, datasize);
  type = bson_parse(arrdata, &ename, &data, &datasize, &endptr2);
  mu_assert_int_eq(bsonDouble, type);
  mu_assert_string_eq("1", ename);
  mu_assert_double_eq(5.05, *((double *)data));
  mu_assert_int_eq(8, datasize);
  type = bson_parse(arrdata, &ename, &data, &datasize, &endptr2);
  mu_assert_int_eq(bsonInt32, type);
  mu_assert_string_eq("2", ename);
  mu_assert_int_eq(1986, *((int32_t *)data));
  mu_assert_int_eq(4, datasize);

  BsonError errcode=1;
  mu_assert_double_eq(5.05, bson_scan_double(arrdata, "1", &errcode));
  mu_assert_int_eq(0, errcode);

  mu_assert_int_eq(1986, bson_scan_int32(arrdata, "2", &errcode));
  mu_assert_int_eq(0, errcode);

  mu_assert_int_eq(0x31, bson_docsize(doc));
  mu_assert_int_eq(1, bson_nelements(doc));

  mu_assert_int_eq(0x26, bson_docsize(arrdata));
  mu_assert_int_eq(3, bson_nelements(arrdata));
}


/* Reproduce example2 using bson_begin_subdoc() and bson_end_subdoc() */
MU_TEST(test_subdoc)
{
  unsigned char doc[1024], *subdoc, expected[] =
    "\x31\x00\x00\x00"
    "\x04BSON\x00"
    "\x26\x00\x00\x00"
    "\x02\x30\x00\x08\x00\x00\x00awesome\x00"
    "\x01\x31\x00\x33\x33\x33\x33\x33\x33\x14\x40"
    "\x10\x32\x00\xc2\x07\x00\x00"
    "\x00";
  int n, m, bufsize=sizeof(doc);

  memset(doc, 255, bufsize);

  n = bson_init_document(doc, bufsize);

  m = bson_begin_subdoc(doc, bufsize-n, "BSON", &subdoc);
  mu_check(m >= 0);
  n += m;
  m = bson_append(subdoc, bufsize-n, bsonString, "0", 7, "awesome");
  mu_check(m >= 0);
  n += m;
  double v1 = 5.05;
  m = bson_append(subdoc, bufsize-n, bsonDouble, "1", -1, &v1);
  mu_check(m >= 0);
  n += m;
  int32_t v2 = 1986;
  m = bson_append(subdoc, bufsize-n, bsonInt32, "2", -1, &v2);
  mu_check(m >= 0);
  n += m;
  m = bson_end_subdoc(doc, bufsize-n, bsonArray);
  mu_check(m >= 0);
  n += m;

  mu_assert_int_eq(0x31, bson_docsize(doc));
  mu_assert_int_eq(0x31, n);
  mu_assert_string_eq((char *)expected, (char *)doc);
}


MU_TEST(test_append_binary)
{
  unsigned char doc[1024], *subdoc;
  int bufsize=sizeof(doc), n, m;

  n = bson_init_document(doc, bufsize);
  m = bson_begin_binary(doc, bufsize-n, "binary", &subdoc);
  mu_check(m >= 0);
  n += m;
  m = bson_append_binary(subdoc, bufsize-n, 4+1, "4444");
  mu_check(m >= 0);
  n += m;
  m = bson_append_binary(subdoc, bufsize-n, 6+1, "666666");
  mu_check(m >= 0);
  n += m;
  m = bson_append_binary(subdoc, bufsize-n, 2+1, "22");
  mu_check(m >= 0);
  n += m;
  m = bson_append_binary(subdoc, bufsize-n, 4+1, "4444");
  mu_check(m >= 0);
  n += m;
  m = bson_end_binary(doc, bufsize-n);
  mu_check(m >= 0);
  n += m;

  printf("\n*** n=%d, docsize=%d\n", n, bson_docsize(doc));
  mu_assert_int_eq(bson_docsize(doc), n);

  FILE *fp = fopen("test-append-binary.bson", "wb");
  mu_check(fp);
  fwrite(doc, n, 1, fp);
  fclose(fp);
}



MU_TEST(test_parse)
{
  unsigned char doc[1024], doc2[128], arr[128], *endptr;
  int bufsize=sizeof(doc), bufsize2=sizeof(doc2), arrsize=sizeof(arr);
  int n, n2, m, type, datasize;
  char *ename;
  void *data;

  /* create document */
  n = bson_init_document(doc, bufsize);
  mu_assert_int_eq(5, n);

  /* append values */
  double v_double = 3.14;
  m = bson_append(doc, bufsize-n, bsonDouble, "v_double", -1, &v_double);
  mu_check(m > 0);
  n += m;

  char *v_str = "a string value";
  m = bson_append(doc, bufsize-n, bsonString, "v_str", strlen(v_str), v_str);
  mu_check(m > 0);
  n += m;

  n2 = bson_init_document(doc2, bufsize2);
  mu_assert_int_eq(5, n2);
  char *v_bin = "\x00\x01\x02\x10";
  m = bson_append(doc2, bufsize2-n2, bsonBinary, "v_bin", sizeof(v_bin), v_bin);
  mu_check(m > 0);
  n2 += m;
  char v_bool = 1;
  m = bson_append(doc2, bufsize2-n2, bsonBool, "v_bool", 1, &v_bool);
  mu_check(m > 0);
  n2 += m;
  mu_assert_int_eq(bson_docsize(doc2), n2);
  m = bson_append(doc, bufsize-n, bsonDocument, "v_doc",
                  bson_docsize(doc2), doc2);
  mu_check(m > 0);
  n += m;

  n2 = bson_init_document(arr, arrsize);
  mu_assert_int_eq(5, n2);
  m = bson_append(arr, arrsize-n2, bsonNull, "0", 0, NULL);
  mu_check(m > 0);
  n2 += m;
  int32_t v_int32 = 42;
  m = bson_append(arr, arrsize-n2, bsonInt32, "1", 4, &v_int32);
  mu_check(m > 0);
  n2 += m;
  mu_assert_int_eq(bson_docsize(arr), n2);
  m = bson_append(doc, bufsize-n, bsonArray, "v_arr",
                  bson_docsize(arr), arr);
  mu_check(m > 0);
  n += m;

  int64_t v_int64 = 123;
  m = bson_append(doc, bufsize-n, bsonInt64, "v_int64", 8, &v_int64);
  mu_check(m > 0);
  n += m;

#ifdef HAVE_FLOAT128
  float128_t v_float128 = 2.718;
  m = bson_append(doc, bufsize-n, bsonInt64, "v_int64", 8, &v_float128);
  mu_check(m > 0);
  n += m;
#endif

  mu_assert_int_eq(n, bson_docsize(doc));

  /* parse values */
  endptr = NULL;
  type = bson_parse(doc, &ename, &data, &datasize, &endptr);
  mu_assert_int_eq(bsonDouble, type);
  mu_assert_string_eq("v_double", ename);
  mu_assert_double_eq(3.14, *((double *)data));
  mu_assert_int_eq(8, datasize);

  type = bson_parse(doc, &ename, &data, &datasize, &endptr);
  mu_assert_int_eq(bsonString, type);
  mu_assert_string_eq("v_str", ename);
  mu_assert_string_eq(v_str, (char *)data);
  mu_assert_int_eq(strlen(v_str), datasize);

  unsigned char *endptr2=NULL;
  void *data2;
  type = bson_parse(doc, &ename, &data, &datasize, &endptr);
  mu_assert_int_eq(bsonDocument, type);
  mu_assert_string_eq("v_doc", ename);
  mu_assert_int_eq(bson_docsize(doc2), datasize);
  type = bson_parse(data, &ename, &data2, &datasize, &endptr2);
  mu_assert_int_eq(bsonBinary, type);
  mu_assert_string_eq("v_bin", ename);
  mu_assert_string_eq(v_bin, (char *)data2);
  mu_assert_int_eq(sizeof(v_bin), datasize);
  type = bson_parse(data, &ename, &data2, &datasize, &endptr2);
  mu_assert_int_eq(bsonBool, type);
  mu_assert_string_eq("v_bool", ename);
  mu_assert_int_eq((int)v_bool, (int)*((char *)data2));
  mu_assert_int_eq(1, datasize);
  type = bson_parse(data, &ename, &data2, &datasize, &endptr2);
  mu_assert_int_eq(0, type);

  type = bson_parse(doc, &ename, &data, &datasize, &endptr);
  mu_assert_int_eq(bsonArray, type);
  mu_assert_string_eq("v_arr", ename);
  mu_assert_int_eq(bson_docsize(arr), datasize);
  endptr2 = NULL;
  type = bson_parse(data, &ename, &data2, &datasize, &endptr2);
  mu_assert_int_eq(bsonNull, type);
  mu_assert_string_eq("0", ename);
  mu_assert_int_eq(0, datasize);
  type = bson_parse(data, &ename, &data2, &datasize, &endptr2);
  mu_assert_int_eq(bsonInt32, type);
  mu_assert_string_eq("1", ename);
  mu_assert_int_eq(4, datasize);
  mu_assert_int_eq(42, *((int32_t *)data2));

  type = bson_parse(doc, &ename, &data, &datasize, &endptr);
  mu_assert_int_eq(bsonInt64, type);
  mu_assert_string_eq("v_int64", ename);
  mu_assert_int_eq(8, datasize);
  mu_assert_int_eq(123, *((int64_t *)data));

#if HAVE_FLOAT128
  type = bson_parse(doc, &ename, &data, &datasize, &endptr);
  mu_assert_int_eq(bsonDecimal128, type);
  mu_assert_string_eq("v_float128", ename);
  mu_assert_int_eq(16, datasize);
  mu_assert_int_eq(2.718, *((float128_t *)data));
#endif

  type = bson_parse(doc, &ename, &data, &datasize, &endptr);
  mu_assert_int_eq(0, type);

  /* Test scan functions */
  BsonError errcode;
  mu_assert_int_eq(42, bson_scan_int32(arr, "1", &errcode));
  mu_assert_int_eq(0, errcode);

  mu_assert_double_eq(3.14, bson_scan_double(doc, "v_double", &errcode));
  mu_assert_int_eq(0, errcode);

  mu_assert_int_eq(123, bson_scan_int64(doc, "v_int64", &errcode));
  mu_assert_int_eq(0, errcode);

  mu_assert_int_eq(0, bson_scan_uint64(doc, "v_int64", &errcode));
  mu_assert_int_eq(bsonTypeError, errcode);

  mu_assert_int_eq(0, bson_scan_uint64(doc, "v_uint64", &errcode));
  mu_assert_int_eq(bsonKeyError, errcode);
}



#include <assert.h>

/* Dedicated tests for issue 556 - inconsistent BSON encoding/decoding */

/* Creates a BSON document containg x as its only encoded value and then
   decode the document and return the decoded value. */
double help_issue556(double x)
{
  BsonError errcode;
  unsigned char doc[1024];
  int n, m, bufsize=sizeof(doc);

  n = bson_init_document(doc, bufsize);
  m = bson_append(doc, bufsize-n, bsonDouble, "value", sizeof(double), &x);
  assert(m > 0);

  double value = bson_scan_double(doc, "value", &errcode);
  return value;
}

MU_TEST(test_issue556) {
#ifndef WINDOWS
  double inf = 1.0/0.0;
  double nan = 0.0/0.0;
#endif
  /* Some interesting doubles to check... */
  double values_to_test[] = {
    1, -1, 3.14, 2.73, -2.3, 1e-6, -1e-6, 1e8, -1e8,
    +0.0, -0.0,
#ifndef WINDOWS
    inf, -inf, nan,
#endif
    0
  };

  /* tests */
  printf("\nTest double encoding/decoding:\n");
  int i, n=sizeof(values_to_test)/sizeof(double);
  for (i=0; i<n; i++) {
    double x = values_to_test[i];
    double v = help_issue556(x);
    printf("  %8.3g..", v);
    mu_assert_double_eq(x, v);
    printf("  ok\n");
  }
  printf("\n");
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_elementsize);
  MU_RUN_TEST(test_example1);
  MU_RUN_TEST(test_example2);
  MU_RUN_TEST(test_subdoc);
  MU_RUN_TEST(test_append_binary);
  MU_RUN_TEST(test_parse);
  MU_RUN_TEST(test_issue556);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
