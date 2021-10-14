#ifndef _GETUUID_H
#define _GETUUID_H
/**
 * @file
 * @brief Generates an UUID from string
 */


/** Length of UUID, excluding terminating NUL. */
#define UUID_LEN 36

/**
 * Writes an UUID to \a buff based on \a id.
 *
 * If \a id is NULL or empty, a new random version 4 UUID is generated.
 * If \a id is an invalid UUID string, a new version 5 sha1-based UUID
 * is generated from \a id using the DNS namespace.
 * Otherwise \a id is copied to \a buff.
 *
 * Length of \a buff must at least 37 bytes (36 for UUID + NUL termination).
 *
 * Returns the UUID version if a new UUID is generated or zero if \a
 * id is simply copied.  On error, -1 is returned.
 */
int getuuid(char *buff, const char *id);


/**
 * Like getuuid(), but takes the the length of `id` as an additional parameter.
 */
int getuuidn(char *buff, const char *id, size_t len);

#endif /* _GETUUID_H */
