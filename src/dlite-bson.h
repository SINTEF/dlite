#ifndef _DLITE_BSON_H
#define _DLITE_BSON_H

/**
  @file
  @brief Built-in support for BSON representation of instances

*/
#include "utils/bson.h"


/**
  Append instance to BSON document.

  Arguments:
    - buf: Pointer to a BSON document to append data to.  The memory pointed
        to must have been initialised with bson_init_document().
    - bufsize: Size of memory segment pointed to by `buf`.  No more than
        `bufsize` bytes will be written.
    - inst: DLite instance to append to BSON document.

  Returns:
    Number of bytes appended (or would have been appended) to `buf`.
    A negative error code is returned on error.
 */
int dlite_bson_append_instance(unsigned char *buf, int bufsize,
                               const DLiteInstance *inst);


/**
  Serialise instance to BSON and return a pointer to a newly allocated
  memory region with the BSON content.

  If `size` is not NULL, it is assigned to the length of the BSON content.

  Returns NULL on error.
*/
unsigned char *dlite_bson_from_instance(const DLiteInstance *inst,
                                        size_t *size);


/**
  Create a new instance from bson document and return it.

  Returns NULL on error.
 */
DLiteInstance *dlite_bson_load_instance(const unsigned char *doc);


#endif  /* _DLITE_BSON_H */
