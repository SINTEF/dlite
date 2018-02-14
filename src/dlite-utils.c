#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compat.h"
#include "dlite.h"
#include "getuuid.h"
#include "err.h"


/* Convenient macros for failing */
#define FAIL(msg) do { err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { err(1, msg, a1); goto fail; } while (0)



/********************************************************************
 * Utility functions
 ********************************************************************/

/*
  Writes an UUID to `buff` based on `id`.

  Whether and what kind of UUID that is generated depends on `id`:
    - If `id` is NULL or empty, a new random version 4 UUID is generated.
    - If `id` is not a valid UUID string, a new version 5 sha1-based UUID
      is generated from `id` using the DNS namespace.
    - Otherwise is `id` already a valid UUID and it is simply copied to
      `buff`.

  Length of `buff` must at least (DLITE_UUID_LENGTH + 1) bytes (36 bytes
  for UUID + NUL termination).

  Returns the UUID version if a new UUID is generated or zero if `id`
  is already a valid UUID.  On error, -1 is returned.
 */
int dlite_get_uuid(char *buff, const char *id)
{
  return getuuid(buff, id);
}


/**
  Returns an unique uri for metadata defined by `name`, `version`
  and `namespace` as a newly malloc()'ed string or NULL on error.

  The returned url is constructed as follows:

      namespace/version/name
 */
char *dlite_join_meta_uri(const char *name, const char *version,
                          const char *namespace)
{
  char *uri = NULL;
  size_t size = 0;
  size_t n = 0;
  if (name) {
    size += strlen(name);
    n++;
  }
  if (version) {
    size += strlen(version);
    n++;
  }
  if (namespace) {
    size += strlen(namespace);
    n++;
  }
  if ((n == 3) && (size > 0)) {
    size += 3;
    if (!(uri = malloc(size))) return err(1, "allocation failure"), NULL;
    snprintf(uri, size, "%s/%s/%s", namespace, version, name);
  }
  return uri;
}

/**
  Splits `metadata` uri into its components.  If `name`, `version` and/or
  `namespace` are not NULL, the memory they points to will be set to a
  pointer to a newly malloc()'ed string with the corresponding value.

  Returns non-zero on error.
 */
int dlite_split_meta_uri(const char *uri, char **name, char **version,
                         char **namespace)
{
  char *p, *q, *namep=NULL, *versionp=NULL, *namespacep=NULL;

  if (!(p = strrchr(uri, '/')))
    FAIL1("invalid metadata uri: '%s'", uri);
  q = p-1;
  while (*q != '/' && q > uri) q--;
  if (q == uri)
    FAIL1("invalid metadata uri: '%s'", uri);

  if (name) {
    if (!(namep = strdup(p + 1))) FAIL("allocation failure");
  }
  if (version) {
    int size = p - q;
    if (!(versionp = malloc(size))) FAIL("allocation failure");
    memcpy(versionp, q + 1, size - 1);
    versionp[size - 1] = '\0';
  }
  if (namespace) {
    int size = q - uri + 1;
    if (!(namespacep = malloc(size))) FAIL("allocation failure");
    memcpy(namespacep, uri, size - 1);
    namespacep[size - 1] = '\0';
  }

  if (name) *name = namep;
  if (version) *version = versionp;
  if (namespace) *namespace = namespacep;
  return 0;
 fail:
  if (namep) free(namep);
  if (versionp) free(versionp);
  if (namespacep) free(namespacep);
  return 1;
}
