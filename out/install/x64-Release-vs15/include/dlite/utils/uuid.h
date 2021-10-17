/*
** Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
** Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
** Digital Equipment Corporation, Maynard, Mass.
** Copyright (c) 1998 Microsoft.
** To anyone who acknowledges that this file is provided "AS IS"
** without any express or implied warranty: permission to use, copy,
** modify, and distribute this file for any purpose is hereby
** granted without fee, provided that the above copyright notices and
** this notice appears in all source code copies, and that none of
** the names of Open Software Foundation, Inc., Hewlett-Packard
** Company, Microsoft, or Digital Equipment Corporation be used in
** advertising or publicity pertaining to distribution of the software
** without specific, written prior permission. Neither Open Software
** Foundation, Inc., Hewlett-Packard Company, Microsoft, nor Digital
** Equipment Corporation makes any representations about the
** suitability of this software for any purpose.
*/
/*
 * Minor modifications by Jesper Friis (2017)
 */

#ifndef __uuid_h__
#define __uuid_h__

#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
# include "integers.h"
#endif

#ifdef _MSC_VER
# if defined(__cplusplus)
#  ifdef DLLEXPORT
#   define DLLEXTERN extern "C" __declspec(dllexport)
#  else
#   define DLLEXTERN extern "C" __declspec(dllimport)
#  endif
# else
#  ifdef DLLEXPORT
#   define DLLEXTERN __declspec(dllexport)
#  else
#   define DLLEXTERN __declspec(dllimport)
#  endif
# endif
#else
# define DLLEXTERN
#endif

typedef struct {
    uint32_t  time_low;
    uint16_t  time_mid;
    uint16_t  time_hi_and_version;
    uint8_t   clock_seq_hi_and_reserved;
    uint8_t   clock_seq_low;
    uint8_t   node[6];
} uuid_s;


/* Namespaces */
extern DLLEXTERN uuid_s NameSpace_DNS, NameSpace_URL, NameSpace_OID,
  NameSpace_X500;



/* uuid_create_md5_from_name -- create a version 3 (MD5) UUID using a
   "name" from a "name space" */
void uuid_create_md5_from_name(
    uuid_s *uuid,         /* resulting UUID */
    uuid_s nsid,          /* UUID of the namespace */
    const void *name,     /* the name from which to generate a UUID */
    int namelen           /* the length of the name */
);

/* uuid_create_sha1_from_name -- create a version 5 (SHA-1) UUID
   using a "name" from a "name space" */
void uuid_create_sha1_from_name(

    uuid_s *uuid,         /* resulting UUID */
    uuid_s nsid,          /* UUID of the namespace */
    const void *name,     /* the name from which to generate a UUID */
    int namelen           /* the length of the name */
);

/* uuid_create_random -- generates a version 4 UUID
   Returns non-zero on error.  */
int uuid_create_random(uuid_s *uuid);

/* uuid_compare --  Compare two UUID's "lexically" and return
        -1   u1 is lexically before u2
         0   u1 is equal to u2
         1   u1 is lexically after u2
   Note that lexical ordering is not temporal ordering!
*/
int uuid_compare(uuid_s *u1, uuid_s *u2);

/* uuid_as_string -- write uuid to string */
void uuid_as_string(uuid_s *uuid, char s[37]);

/* uuid_from_string -- set uuid from string `s` of length `len`.
   Returns non-zero if `s` is not formatted as a valid uuid.
   If `uuid` is NULL, only format checking is performed. */
int uuid_from_string(uuid_s *uuid, const char *s, size_t len);

#endif /* !__uuid_h__ */
