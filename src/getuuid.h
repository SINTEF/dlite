#ifndef _GETUUID_H
#define _GETUUID_H
/**
 * @file
 * @brief Generates an UUID from string
 */


/** Length of UUID, excluding terminating NUL. */
#define UUID_LEN 36

/**
  Writes an UUID to `buff` based on `id`.

  It follow the follow heuristics:
  - If `id` is NULL or empty, a new random version 4 UUID is generated.
  - If `id` matches `<NAMESPACE>/<VERSION/<NAME>/<UUID>` then it returns
    the <UUID> part.  `id` may optionally end with a final hash (#) or
    slash (/), which will be ignored.
  - If `id` is an invalid UUID string, a new version 5 sha1-based UUID
    is generated from `id` using the DNS namespace.  Any optional final
    hash (#) or slash (/) will be stripped off.
  - Otherwise `id` is a valid UUID and it is copied as-is to `buff`.

  Length of `buff` must at least 37 bytes (36 for UUID + NUL termination).

  Returns the UUID version if a new UUID is generated or zero if
  `id` is simply copied.  On error, -1 is returned.
 */
int getuuid(char *buff, const char *id);


/**
 * Like getuuid(), but takes the the length of `id` as an additional parameter.
 */
int getuuidn(char *buff, const char *id, size_t len);

#endif /* _GETUUID_H */
