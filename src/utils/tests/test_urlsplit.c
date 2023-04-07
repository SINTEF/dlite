#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "urlsplit.h"

#include "minunit/minunit.h"


MU_TEST(test_urlsplit)
{
  char *url, buf[256];
  int n;
  UrlComponents comp;

  url = "http://www.example.com/questions/3456/my-document";
  n = urlsplit(url, &comp);
  mu_assert_int_eq(49, n);
  mu_assert_int_eq(4, comp.scheme_len);
  mu_assert_strn_eq("http", comp.scheme, 4);
  mu_assert_int_eq(15, comp.authority_len);
  mu_assert_strn_eq("www.example.com", comp.authority, 15);
  mu_assert_int_eq(0, comp.userinfo_len);
  mu_assert_ptr_eq(NULL, comp.userinfo);
  mu_assert_int_eq(15, comp.host_len);
  mu_assert_strn_eq("www.example.com", comp.host, 15);
  mu_assert_int_eq(27, comp.path_len);
  mu_assert_string_eq("/questions/3456/my-document", comp.path);
  mu_assert_int_eq(0, comp.query_len);
  mu_assert_string_eq(NULL, comp.query);
  mu_assert_int_eq(0, comp.fragment_len);
  mu_assert_string_eq(NULL, comp.fragment);

  n = urljoin(buf, sizeof(buf), &comp);
  mu_assert_int_eq(49, n);
  mu_assert_string_eq(url, buf);

  url = "http://www.example.com";
  n = urlsplit(url, &comp);
  mu_assert_int_eq(22, n);
  mu_assert_int_eq(4, comp.scheme_len);
  mu_assert_strn_eq("http", comp.scheme, 4);
  mu_assert_int_eq(15, comp.authority_len);
  mu_assert_string_eq("www.example.com", comp.authority);
  mu_assert_int_eq(15, comp.host_len);
  mu_assert_string_eq("www.example.com", comp.host);
  mu_assert_int_eq(0, comp.path_len);
  mu_assert_string_eq("", comp.path);

  n = urljoin(buf, sizeof(buf), &comp);
  mu_assert_int_eq(22, n);
  mu_assert_string_eq(url, buf);

  url = "http://www.example.com/";
  n = urlsplit(url, &comp);
  mu_assert_int_eq(23, n);
  mu_assert_int_eq(1, comp.path_len);
  mu_assert_string_eq("/", comp.path);

  n = urljoin(buf, sizeof(buf), &comp);
  mu_assert_int_eq(23, n);
  mu_assert_string_eq(url, buf);

  url = "ftp://anonymous@192.168.0.39/story.txt";
  n = urlsplit(url, &comp);
  mu_assert_int_eq(38, n);
  mu_assert_int_eq(3, comp.scheme_len);
  mu_assert_strn_eq("ftp", comp.scheme, 3);
  mu_assert_int_eq(22, comp.authority_len);
  mu_assert_strn_eq("anonymous@192.168.0.39", comp.authority, 22);
  mu_assert_int_eq(9, comp.userinfo_len);
  mu_assert_strn_eq("anonymous", comp.userinfo, 9);
  mu_assert_int_eq(12, comp.host_len);
  mu_assert_strn_eq("192.168.0.39", comp.host, 12);
  mu_assert_int_eq(10, comp.path_len);
  mu_assert_strn_eq("/story.txt", comp.path, 10);

  n = urljoin(buf, sizeof(buf), &comp);
  mu_assert_int_eq(38, n);
  mu_assert_string_eq(url, buf);

  url = "mongodb+srv://guest:guest@localhost:27017?db=A&coll=C";
  n = urlsplit(url, &comp);
  mu_assert_int_eq(53, n);
  mu_assert_int_eq(11, comp.scheme_len);
  mu_assert_strn_eq("mongodb+srv", comp.scheme, 11);
  mu_assert_int_eq(27, comp.authority_len);
  mu_assert_strn_eq("guest:guest@localhost:27017", comp.authority, 27);
  mu_assert_int_eq(11, comp.userinfo_len);
  mu_assert_strn_eq("guest:guest", comp.userinfo, 11);
  mu_assert_int_eq(9,  comp.host_len);
  mu_assert_strn_eq("localhost", comp.host, 9);
  mu_assert_int_eq(5,  comp.port_len);
  mu_assert_strn_eq("27017", comp.port, 5);
  mu_assert_int_eq(0,  comp.path_len);
  mu_assert_strn_eq("", comp.path, 0);
  mu_assert_int_eq(11, comp.query_len);
  mu_assert_strn_eq("db=A&coll=C", comp.query, 11);

  n = urljoin(buf, sizeof(buf), &comp);
  mu_assert_int_eq(53, n);
  mu_assert_string_eq(url, buf);

  url = "file:~/.bashrc";
  n = urlsplit(url, &comp);
  mu_assert_int_eq(14, n);
  mu_assert_int_eq(4, comp.scheme_len);
  mu_assert_strn_eq("file", comp.scheme, 4);
  mu_assert_int_eq(0, comp.authority_len);
  mu_assert_ptr_eq(NULL, comp.authority);
  mu_assert_int_eq(0, comp.userinfo_len);
  mu_assert_ptr_eq(NULL, comp.userinfo);
  mu_assert_int_eq(0, comp.host_len);
  mu_assert_ptr_eq(NULL, comp.host);
  mu_assert_int_eq(9, comp.path_len);
  mu_assert_string_eq("~/.bashrc", comp.path);
  mu_assert_int_eq(0, comp.query_len);
  mu_assert_ptr_eq(NULL, comp.query);

  n = urljoin(buf, sizeof(buf), &comp);
  mu_assert_int_eq(14, n);
  mu_assert_string_eq(url, buf);

  url = "http://localhost#frag";
  n = urlsplit(url, &comp);
  mu_assert_int_eq(21, n);
  mu_assert_int_eq(4,  comp.scheme_len);
  mu_assert_strn_eq("http", comp.scheme, 4);
  mu_assert_int_eq(9,  comp.authority_len);
  mu_assert_strn_eq("localhost", comp.authority, 9);
  mu_assert_int_eq(0,  comp.userinfo_len);
  mu_assert_ptr_eq(NULL, comp.userinfo);
  mu_assert_int_eq(9,  comp.host_len);
  mu_assert_strn_eq("localhost", comp.host, 9);
  mu_assert_int_eq(0,  comp.path_len);
  mu_assert_strn_eq("", comp.path, 0);
  mu_assert_int_eq(0,  comp.query_len);
  mu_assert_ptr_eq(NULL,  comp.query);
  mu_assert_int_eq(4,  comp.fragment_len);
  mu_assert_strn_eq("frag", comp.fragment, 4);

  n = urljoin(buf, sizeof(buf), &comp);
  mu_assert_int_eq(21, n);
  mu_assert_string_eq(url, buf);

  mu_assert_int_eq(0, urlsplit("..", NULL));
  mu_assert_int_eq(0, urlsplit("ftp", NULL));
  mu_assert_int_eq(0, urlsplit("ftp@", NULL));
  mu_assert_int_eq(0, urlsplit("ftp~ssh:", NULL));
  mu_assert_int_eq(4, urlsplit("ftp: ", NULL));
  mu_assert_int_eq(4, urlsplit("ftp: /", NULL));
}


MU_TEST(test_pct_encode)
{
  char buf[10];
  int n;

  n = pct_encode(buf, sizeof(buf), "a={}");
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("a=%7B%7D", buf);

  n = pct_encode(buf, sizeof(buf), "a=Å");
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("a=%C3%85", buf);

  n = pct_encode(buf, 3, "a={}");
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("a=", buf);

  n = pct_encode(buf, 4, "a={}");
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("a=", buf);

  n = pct_encode(buf, 5, "a={}");
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("a=", buf);

  n = pct_encode(buf, 6, "a={}");
  mu_assert_int_eq(8, n);
  mu_assert_string_eq("a=%7B", buf);

  n = pct_nencode(buf, sizeof(buf), "a={}", 3);
  mu_assert_int_eq(5, n);
  mu_assert_string_eq("a=%7B", buf);
}


MU_TEST(test_pct_decode)
{
  char buf[64];
  int n;

  n = pct_decode(buf, 4, "a=%C3%85");
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("a=", buf);

  n = pct_decode(buf, sizeof(buf), "a=%7B%7D");
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("a={}", buf);

  n = pct_decode(buf, sizeof(buf), "a=%C3%85");
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("a=Å", buf);

  n = pct_decode(buf, sizeof(buf), "a=%e2%82%ac");
  mu_assert_int_eq(5, n);
  mu_assert_string_eq("a=€", buf);

  n = pct_decode(buf, 3, "a=%7B%7D");
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("a=", buf);

  n = pct_decode(buf, 4, "a=%7B%7D");
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("a={", buf);

  n = pct_decode(buf, 4, "a=%C3%85");
  mu_assert_int_eq(4, n);
  mu_assert_string_eq("a=", buf);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_urlsplit);
  MU_RUN_TEST(test_pct_encode);
  MU_RUN_TEST(test_pct_decode);
}


int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
