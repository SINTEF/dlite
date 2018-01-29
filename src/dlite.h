#ifndef _DLITE_H
#define _DLITE_H

/**
  @file
  @brief Main header file for dlite
*/

#include "dlite-type.h"
#include "dlite-storage.h"

#define DLITE_UUID_LENGTH 36



/**
  @name General dlite utility functions
  @{
 */

/**
  Writes an UUID to \a buff based on \a id.

  Whether and what kind of UUID that is generated depends on \a id:
    - If \a id is NULL or empty, a new random version 4 UUID is generated.
    - If \a id is not a valid UUID string, a new version 5 sha1-based UUID
      is generated from \a id using the DNS namespace.
    - Otherwise is \a id already a valid UUID and it is simply copied to
      \a buff.

  Length of \a buff must at least (DLITE_UUID_LENGTH + 1) bytes (36 bytes
  for UUID + NUL termination).

  Returns the UUID version if a new UUID is generated or zero if \a id
  is already a valid UUID.  On error, -1 is returned.
 */
int dlite_get_uuid(char *buff, const char *id);


/**
  Returns an unique url for metadata defined by \a name, \a version
  and \a namespace as a newly malloc()'ed string or NULL on error.

  The returned url is constructed as follows:

      namespace/version/name
 */
char *dlite_join_metadata(const char *name, const char *version,
                          const char *namespace);

/**
  Splits \a metadata url into its components.  If \a name, \a version and/or
  \a namespace are not NULL, the memory they points to will be set to a
  pointer to a newly malloc()'ed string with the corresponding value.

  Returns non-zero on error.
*/
int dlite_split_metadata(const char *metadata, char **name, char **version,
                         char **namespace);


/** @} */


#endif /* _DLITE_H */
