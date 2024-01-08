#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sha3.h"
#include "strutils.h"
#include "test_macros.h"

#include "minunit/minunit.h"


const char *testsha3(int bits, void *data, size_t n, enum SHA3_FLAGS flags)
{
  static char buf[256];
  const unsigned char *hash;
  sha3_context c;
  assert((int)sizeof(buf) > 2*bits/8);

  if (sha3_Init(&c, bits)) return NULL;
  sha3_SetFlags(&c, flags);
  sha3_Update(&c, data, n);
  hash = sha3_Finalize(&c);
  strhex_encode(buf, sizeof(buf), hash, bits/8);
  return buf;
}



MU_TEST(test_sha256)
{
  // Python: hashlib.sha3_256(b'abc').hexdigest()
  mu_assert_string_eq(
    "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532",
    testsha3(256, "abc", 3, SHA3_FLAGS_NONE));

  /* Check that the result is reproduced if input is splitted up in several
     chunks... */
  sha3_context c;
  const unsigned char *hash;
  char buf[64+1];
  sha3_Init256(&c);
  sha3_Update(&c, "a", 1);
  sha3_Update(&c, "bc", 2);
  hash = sha3_Finalize(&c);
  strhex_encode(buf, sizeof(buf), hash, 32);
  mu_assert_string_eq(
    "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532",
    buf);


  mu_assert_string_eq(
    "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45",
    testsha3(256, "abc", 3, SHA3_FLAGS_KECCAK));
}



MU_TEST(test_sha384)
{
  // Python: hashlib.sha3_384(b'abc').hexdigest()
  mu_assert_string_eq(
    "ec01498288516fc926459f58e2c6ad8df9b473cb0fc08c2596da7cf0e49be4b2"
    "98d88cea927ac7f539f1edf228376d25",
    testsha3(384, "abc", 3, SHA3_FLAGS_NONE));

  mu_assert_string_eq(
    "f7df1165f033337be098e7d288ad6a2f74409d7a60b49c36642218de161b1f99"
    "f8c681e4afaf31a34db29fb763e3c28e",
    testsha3(384, "abc", 3, SHA3_FLAGS_KECCAK));
}



MU_TEST(test_sha512)
{
  // Python: hashlib.sha3_512(b'abc').hexdigest()
  mu_assert_string_eq(
    "b751850b1a57168a5693cd924b6b096e08f621827444f70d884f5d0240d2712e"
    "10e116e9192af3c91a7ec57647e3934057340b4cf408d5a56592f8274eec53f0",
    testsha3(512, "abc", 3, SHA3_FLAGS_NONE));

  mu_assert_string_eq(
    "18587dc2ea106b9a1563e32b3312421ca164c7f1f07bc922a9c83d77cea3a1e5"
    "d0c69910739025372dc14ac9642629379540c17e2a65b19d77aa511a9d00bb96",
    testsha3(512, "abc", 3, SHA3_FLAGS_KECCAK));
}



/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_sha256);
  MU_RUN_TEST(test_sha384);
  MU_RUN_TEST(test_sha512);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
