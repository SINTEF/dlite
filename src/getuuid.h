#ifndef _GETUUID_H
#define _GETUUID_H
/**
 * @file
 * @brief Generates an UUID from string
 */


/** Length of UUID, excluding terminating NUL. */
#define UUID_LEN 36

/** Return values */
#define UUID_ERROR   -1  //!< error
#define UUID_COPY     0  //!< copied UUID from input as-is
#define UUID_RANDOM   4  //!< random version 4 UUID
#define UUID_HASH     5  //!< version 5 sha1-based UUID using the DNS namespace
#define UUID_EXTRACT 10  //!< UUID extracted input: [metadata URI]/[UUID]


/**
  Writes an UUID to `buff` based on `id`.

  It follow the follow heuristics:
  - If `id` is NULL or empty, a new random version 4 UUID is generated.
    Return: UUID_RANDOM
  - If `id` is a valid UUID, it is copied as-is to `buff`.
    Return: UUID_COPY
  - If `id` matches `[URI]/[UUID]` then it returns the [UUID] part.
    `id` may optionally end with a final hash (#) or slash (/), which will
    be ignored.
    Return: UUID_EXTRACT
  - Otherwise is `id` an invalid UUID string.  A new version 5 sha1-based
    UUID is generated from `id` using the DNS namespace.  Any optional final
    hash (#) or slash (/) will be stripped off.
    Return: UUID_HASH

  Length of `buff` must at least 37 bytes (36 for UUID + NUL termination).

  Returns one of the codes UUID_RANDOM, UUID_COPY, UUID_EXTRACT or UUID_HASH.
  On error -1 is returned.
 */
int getuuid(char *buff, const char *id);


/**
  Like getuuid(), but takes the the length of `id` as an additional parameter.
 */
int getuuidn(char *buff, const char *id, size_t len);


/**
  Returns non-zero if `s` is a valid UUID.
 */
int isuuid(const char *s);


/**
  Returns non-zero if `s` matches `[URI]/[UUID]. `len` is the length of `s`.
  An optional final hash or slash will be ignored.
*/
int isinstanceuri(const char *s, int len);

#endif /* _GETUUID_H */
