/*
 * test_uuid.c
 * Copyright (C) 2017 SINTEF Materials and Chemistry
 * By Jesper Friis <jesper.friis@sintef.no>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <string.h>

#include "integers.h"
#include "byteswap.h"
#include "byteorder.h"

#include "md5.h"
#include "sha1.h"
#include "uuid.h"

/* Get rid of MSVC warnings */
#ifdef _MSC_VER
# pragma warning(disable: 4217 4996)
#endif



int test_bswap_16()
{
  if (bswap_16(0x1122) != 0x2211) return 1;
  return 0;
}

int test_bswap_32()
{
  if (bswap_32(0x11223344) != 0x44332211) return 1;
  return 0;
}

#ifdef HAVE_INT64
int test_bswap_64()
{
  if (bswap_64(0x1122334455667788) != 0x8877665544332211) return 1;
  return 0;
}
#endif


int test_md5()
{
  MD5_CTX md5;
  size_t i;
  unsigned char digest[16];
  char s[33], **p;
  char *messages[] = {"www.widgets.com",
                      "En af dem der red med fane",
                      NULL};
  MD5Init(&md5);
  for (p=messages; *p; p++)
    MD5Update(&md5, *p, (unsigned long)strlen(*p));
  MD5Final(digest, &md5);

  for (i=0; i<sizeof(digest); i++)
    sprintf(s + 2*i, "%2.2x", digest[i]);

  return strcmp(s, "b1283d7fe3871c2d61c031af615a7312");
}


int test_sha1()
{
  SHA1_CTX sha1;
  size_t i;
  unsigned char digest[20];
  char s[41], **p;
  char *messages[] = {"www.widgets.com",
                      "En af dem der red med fane",
                      NULL};
  SHA1Init(&sha1);
  for (p=messages; *p; p++)
    SHA1Update(&sha1, (unsigned char *)(*p), (uint32_t)strlen(*p));
  SHA1Final(digest, &sha1);

  for (i=0; i<sizeof(digest); i++)
    sprintf(s + 2*i, "%2.2x", digest[i]);

  return strcmp(s, "75bd58d47182594884598b0f2c84d7ef59bc461f");
}


int test_uuid3()
{
  uuid_s u;
  char *s = "www.widgets.com";
  char uuid[37];

  uuid_create_md5_from_name(&u, NameSpace_DNS, s, (int)strlen(s));
  uuid_as_string(&u, uuid);
  return strcmp(uuid, "3d813cbb-47fb-32ba-91df-831e1593ac29");
}

int test_uuid5()
{
  uuid_s u;
  char *s = "www.widgets.com";
  char uuid[37];

  uuid_create_sha1_from_name(&u, NameSpace_DNS, s, (int)strlen(s));
  uuid_as_string(&u, uuid);

  return strcmp(uuid, "21f7f8de-8051-5b89-8680-0195ef798b6a");
}

int test_uuid_as_string()
{
  char uuid[37];
  uuid_as_string(&NameSpace_DNS, uuid);
  return strcmp(uuid, "6ba7b810-9dad-11d1-80b4-00c04fd430c8");
}

int test_uuid_from_string()
{
  uuid_s u;
  if (uuid_from_string(&u, "6ba7b811-9dad-11d1-80b4-00c04fd430c8", 36))
    return 1;
  return uuid_compare(&u, &NameSpace_URL);
}


/***********************************************************************/

int nerr = 0;  /* global error count */

#define Assert(name, cond)                      \
  do {                                          \
    if (cond) {                                 \
      printf("%-18s : OK\n", (name));           \
    } else {                                    \
      nerr++;                                   \
      printf("%-18s : Failed\n", (name));       \
    }                                           \
  } while (0)


int main() {
  Assert("bswap_16", test_bswap_16() == 0);
  Assert("bswap_32", test_bswap_32() == 0);
#ifdef HAVE_INT64
  Assert("bswap_64", test_bswap_64() == 0);
#endif
  Assert("md5", test_md5() == 0);
  Assert("sha1", test_sha1() == 0);
  Assert("uuid3", test_uuid3() == 0);
  Assert("uuid5", test_uuid5() == 0);
  Assert("uuid_as_string", test_uuid_as_string() == 0);
  Assert("uuid_from_string", test_uuid_from_string() == 0);

  return nerr;
}
