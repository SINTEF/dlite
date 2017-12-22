#include <stdlib.h>
#include <string.h>

#include "uuid.h"
#include "uuid4.h"

#include "getuuid.h"


/* Writes an UUID to \a buff based on \a id.
 *
 * If \a id is NULL or empty, a new random version 4 UUID is generated.
 * If \a id is an invalid UUID string, a new version 5 sha1-based UUID
 * is generated from \id using the DNS namespace.
 * Otherwise \a id is copied to \a buff.
 *
 * Length of \a buff must at least ``UUID_LEN + 1`` (including the
 * terminating NUL).
 *
 * Returns nonzero on error.
 */
int getuuid(char *buff, const char *id)
{
  int status=0;
  uuid_s uuid;

  if (!id) {
    status = uuid4_generate(buff);
  } else if (uuid_from_string(NULL, id)) {
    uuid_create_sha1_from_name(&uuid, NameSpace_DNS, id, strlen(id));
    uuid_as_string(&uuid, buff);
  } else {
    strncpy(buff, id, UUID_LEN);
    buff[UUID_LEN] = '\0';
  }
  return status;
}
