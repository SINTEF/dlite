/*
 * uuid.c
 * Copyright (C) 2016 Kurten Chan <chinkurten@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */
/*
 * Modified by Jesper Friis (2017)
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "byteorder.h"
#include "md5.h"
#include "sha1.h"
#include "uuid4.h"
#include "uuid.h"

/* Get rid of MSVS warnings */
#if defined WIN32 || defined _WIN32 || defined __WIN32__
# ifndef CROSS_TARGET
#  pragma warning(disable: 4273 4996)
# endif
#endif


typedef uint64_t uuid_sime_t;
typedef struct {
    char nodeID[6];
} uuid_node_t;


/* various forward declarations */
static void format_uuid_v3or5(uuid_s *uuid, unsigned char hash[16], int v);

uuid_s NameSpace_DNS = { /* 6ba7b810-9dad-11d1-80b4-00c04fd430c8 */
    0x6ba7b810,
    0x9dad,
    0x11d1,
    0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}
};

uuid_s NameSpace_URL = { /* 6ba7b811-9dad-11d1-80b4-00c04fd430c8 */
    0x6ba7b811,
    0x9dad,
    0x11d1,
    0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}
};

uuid_s NameSpace_OID = { /* 6ba7b812-9dad-11d1-80b4-00c04fd430c8 */
    0x6ba7b812,
    0x9dad,
    0x11d1,
    0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}
};

uuid_s NameSpace_X500 = { /* 6ba7b814-9dad-11d1-80b4-00c04fd430c8 */
    0x6ba7b814,
    0x9dad,
    0x11d1,
    0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}
};


/* Returns a single hex character converted to an integer */
uint8_t hexdigit(char hex)
{
  return (hex <= '9') ? hex - '0' : toupper(hex) - 'A' + 10;
}

/* Reads two hex characters from "hex" and returns the resulting byte. */
uint8_t hexbyte(const char* hex)
{
  return (hexdigit(*hex) << 4) | hexdigit(*(hex+1));
}


/* uuid_create_md5_from_name -- create a version 3 (MD5) UUID using a
   "name" from a "name space" */
void uuid_create_md5_from_name(uuid_s *uuid, uuid_s nsid, const void *name,
                               int namelen)
{
    MD5_CTX c;
    unsigned char hash[16];
    uuid_s net_nsid;

    /* put name space ID in network byte order so it hashes the same
       no matter what endian machine we're on */
    net_nsid = nsid;
    net_nsid.time_low = htobe32(net_nsid.time_low);
    net_nsid.time_mid = htobe16(net_nsid.time_mid);
    net_nsid.time_hi_and_version = htobe16(net_nsid.time_hi_and_version);

    MD5Init(&c);
    MD5Update(&c, &net_nsid, sizeof(net_nsid));
    MD5Update(&c, name, namelen);
    MD5Final(hash, &c);

    /* the hash is in network byte order at this point */
    format_uuid_v3or5(uuid, hash, 3);
}

void uuid_create_sha1_from_name(uuid_s *uuid, uuid_s nsid, const void *name,
                                int namelen)
{
    SHA1_CTX c;
    unsigned char hash[20];
    uuid_s net_nsid;

    /* put name space ID in network byte order so it hashes the same
       no matter what endian machine we're on */
    net_nsid = nsid;
    net_nsid.time_low = htobe32(net_nsid.time_low);
    net_nsid.time_mid = htobe16(net_nsid.time_mid);
    net_nsid.time_hi_and_version = htobe16(net_nsid.time_hi_and_version);

    SHA1Init(&c);
    SHA1Update(&c, (unsigned char *)(&net_nsid), sizeof net_nsid);
    SHA1Update(&c, name, namelen);
    SHA1Final(hash, &c);

    /* the hash is in network byte order at this point */
    format_uuid_v3or5(uuid, hash, 5);
}

/* format_uuid_v3or5 -- make a UUID from a (pseudo)random 128-bit
   number */
void format_uuid_v3or5(uuid_s *uuid, unsigned char hash[16], int v)
{
    /* convert UUID to local byte order */
    memcpy(uuid, hash, sizeof *uuid);
    uuid->time_low = be32toh(uuid->time_low);
    uuid->time_mid = be16toh(uuid->time_mid);
    uuid->time_hi_and_version = be16toh(uuid->time_hi_and_version);

    /* put in the variant and version bits */
    uuid->time_hi_and_version &= 0x0FFF;
    uuid->time_hi_and_version |= (v << 12);
    uuid->clock_seq_hi_and_reserved &= 0x3F;
    uuid->clock_seq_hi_and_reserved |= 0x80;
}

/* uuid_create_random -- generates a version 4 UUID
   Returns non-zero on error.  */
int uuid_create_random(uuid_s *uuid)
{
  char buf[UUID4_LEN];
  if (uuid4_generate(buf))
    return 1;
  if (uuid_from_string(uuid, buf, 36))
    return 1;
  return 0;
}


/* uuid_compare --  Compare two UUID's "lexically" and return */
#define CHECK(f1, f2) if (f1 != f2) return f1 < f2 ? -1 : 1;
int uuid_compare(uuid_s *u1, uuid_s *u2)
{
    int i;

    CHECK(u1->time_low, u2->time_low);
    CHECK(u1->time_mid, u2->time_mid);
    CHECK(u1->time_hi_and_version, u2->time_hi_and_version);
    CHECK(u1->clock_seq_hi_and_reserved, u2->clock_seq_hi_and_reserved);
    CHECK(u1->clock_seq_low, u2->clock_seq_low)
    for (i = 0; i < 6; i++) {
        if (u1->node[i] < u2->node[i])
            return -1;
        if (u1->node[i] > u2->node[i])
            return 1;
    }
    return 0;
}

/* uuid_as_string -- write uuid to string (of length 36+1 bytes) */
void uuid_as_string(uuid_s *uuid, char s[37])
{
  int i, n=0;
  n += sprintf(s + n, "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-",
               uuid->time_low, uuid->time_mid,
               uuid->time_hi_and_version,
               uuid->clock_seq_hi_and_reserved,
               uuid->clock_seq_low);
  for (i = 0; i < 6; i++)
    n += sprintf(s + n, "%2.2x", uuid->node[i]);
  assert(n == 36);
  s[n] = '\0';
}

/* uuid_from_string -- set uuid from string s. Returns non-zero if s
   is not formatted as a valid uuid.  If uuid is none, only format
   checking is done. */
int uuid_from_string(uuid_s *uuid, const char *s, size_t len)
{
  uuid_s u;
  int i, n;
  if (len != 36)
    return 1;

  if (sscanf(s, "%8x-%4hx-%4hx-%n",
             &u.time_low, &u.time_mid, &u.time_hi_and_version, &n) != 3)
    return 1;
  assert(n == 19);

  u.clock_seq_hi_and_reserved = hexbyte(s + n);
  n += 2;
  u.clock_seq_low = hexbyte(s + n);
  n += 2;
  n++;  /* hyphen */
  for (i=0; i<6; i++) {
    u.node[i] = hexbyte(s + n);
    n += 2;
  }

  if (uuid)
    memcpy(uuid, &u, sizeof(uuid_s));

  return 0;
}

/* Returns non-zero if `s` is a valid UUID. */
int isuuid(const char *s)
{
  int i;
  for (i=0; i<8; i++) if (!isxdigit(*(s++))) return 0;
  if (*(s++) != '-') return 0;
  for (i=0; i<4; i++) if (!isxdigit(*(s++))) return 0;
  if (*(s++) != '-') return 0;
  for (i=0; i<4; i++) if (!isxdigit(*(s++))) return 0;
  if (*(s++) != '-') return 0;
  for (i=0; i<4; i++) if (!isxdigit(*(s++))) return 0;
  if (*(s++) != '-') return 0;
  for (i=0; i<12; i++) if (!isxdigit(*(s++))) return 0;
  return 1;
}
