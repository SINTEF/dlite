#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "config.h"

#include "utils/strutils.h"
#include "utils/uuid.h"
#include "utils/uuid4.h"
#include "getuuid.h"


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

/* Returns non-zero if `s` matches <URI>/<UUID>. `len` is the length of `s`.
   An optional final hash or slash will be ignored.*/
int isinstanceuri(const char *s, int len)
{
  if (!len) len = strlen(s);
  if (len < UUID_LEN + 10) return 0;
  if (strchr("#/", s[len-1])) len--;
  len -= UUID_LEN;
  if (!isuuid(s+len)) return 0;
  if (s[--len] != '/') return 0;
  if (strcatjspn(s, strcatPercent) < len) return 0;
  if ((int)strcspn(s, ":") >= len) return 0;
  return 1;
}


/*
  Writes an UUID to `buff` based on `id`.

  It follow the follow heuristics:
  - If `id` is NULL or empty, a new random version 4 UUID is generated.
    Return: UUID_RANDOM
  - If `id` is a valid UUID, it is copied as-is to `buff`.
    Return: UUID_COPY
  - If `id` matches `<URI>/<UUID>` then it returns the <UUID> part.
    `id` may optionally end with a final hash (#) or slash (/), which will
    be ignored.
    Return: UUID_EXTRACT
  - Otherwise is `id` is an invalid UUID string.  A new version 5 sha1-based
    UUID is generated from `id` using the DNS namespace.  Any optional final
    hash (#) or slash (/) will be stripped off.
    Return: UUID_HASH

  Length of `buff` must at least 37 bytes (36 for UUID + NUL termination).

  Returns one of the codes UUID_RANDOM, UUID_COPY, UUID_EXTRACT or UUID_HASH.
  On error -1 is returned.
 */
int getuuid(char *buff, const char *id)
{
  return getuuidn(buff, id, (id) ? strlen(id) : 0);
}


/*
 * Like getuuid(), but takes the the length of `id` as an additional parameter.
 */
int getuuidn(char *buff, const char *id, size_t len)
{
  int i, version;
  uuid_s uuid;

  if (len == 0) id = NULL;

  if (!id || !*id) {
    int status = uuid4_generate(buff);
    if (status) return -1;
    version = UUID_RANDOM;
  } else if (((len == UUID_LEN) ||
              (len == UUID_LEN+1 && strchr("#/", id[len]))) &&
             isuuid(id)) {
    strncpy(buff, id, UUID_LEN);
    buff[UUID_LEN] = '\0';
    version = UUID_COPY;
  } else if (isinstanceuri(id, len)) {
    if (id[len-1] && strchr("#/", id[len-1])) len--;
    len -= UUID_LEN;
    assert(len > 0);
    strncpy(buff, id+len, UUID_LEN);
    version = UUID_EXTRACT;
  } else {
    uuid_create_sha1_from_name(&uuid, NameSpace_DNS, id, len);
    uuid_as_string(&uuid, buff);
    version = UUID_HASH;
  }

  /* For reprodusability, always convert to lower case */
  for (i=0; i < UUID_LEN; i++)
    buff[i] = tolower(buff[i]);

  return version;
}
