/* A set of utility function for serialising and deserialising dlite instances
   to/from JSON.
 */

#ifndef _DLITE_PRINT_H
#define _DLITE_PRINT_H


typedef enum {
  dlitePrintUUID=1,        /*!< Whether to include uuid */
  dlitePrintMetaAsData=2,  /*!< Whether to print entities simplified */
} DLitePrintFlag;

/**
  Serialise instance `inst` to `dest`, formatted as JSON.

  No more than `size` bytes are written to `dest` (incl. the
  terminating NUL).

  Returns number of bytes written to `dest`.  If the output is
  truncated because it exceeds `size`, the number of bytes that would
  have been written if `size` was large enough is returned.  On error, a
  negative value is returned.
*/
int dlite_sprint(char *dest, size_t size, DLiteInstance *inst,
                 int indent, DLitePrintFlag flags);


/**
  Like dlite_sprint(), but prints to allocated buffer.

  Prints to position `pos` in `*dest`, which should point to a buffer
  of size `*size`.  `*dest` is reallocated if needed.

  Returns number or bytes written or a negative number on error.
 */
int dlite_asprint(char **dest, size_t *size, size_t pos, DLiteInstance *inst,
                  int indent, DLitePrintFlag flags);


/**
  Like dlite_sprint(), but returns allocated buffer with serialised instance.
 */
char *dlite_aprint(DLiteInstance *inst, int indent, DLitePrintFlag flags);


/**
  Like dlite_sprint(), but prints to stream `fp`.

  Returns number or bytes printed or a negative number on error.
 */
int dlite_fprint(FILE *fp, DLiteInstance *inst, int indent,
                 DLitePrintFlag flags);



DLiteInstance *dlite_sscan(const char *src, const char *id);

DLiteInstance *dlite_fscan(FILE *fp, const char *id);



#endif /*_ DLITE_PRINT_H */
