#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "config.h"

#include "utils/uuid.h"
#include "utils/uuid4.h"
#include "getuuid.h"


/*
 * Writes an UUID to `buff` based on `id`.
 *
 * If `id` is NULL or empty, a new random version 4 UUID is generated.
 * If `id` is an invalid UUID string, a new version 5 sha1-based UUID
 * is generated from `id` using the DNS namespace.
 * Otherwise `id` is copied to `buff`.
 *
 * Length of `buff` must at least 37 bytes (36 for UUID + NUL termination).
 *
 * Returns the UUID version, if a new UUID is generated or zero if
 * `id` is simply copied.  On error, -1 is returned.
 */
int getuuid(char *buff, const char *id)
{
  int version;
  uuid_s uuid;

  if (!id) {
    int status = uuid4_generate(buff);
    version = (status == 0) ? 4 : -1;
  } else if (uuid_from_string(NULL, id)) {
    uuid_create_sha1_from_name(&uuid, NameSpace_DNS, id, strlen(id));
    uuid_as_string(&uuid, buff);
    version = 5;
  } else {
    strncpy(buff, id, UUID_LEN);
    buff[UUID_LEN] = '\0';
    version = 0;
  }

  return version;
}
