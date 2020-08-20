#ifndef _DLITE_TYPE_CAST_H
#define _DLITE_TYPE_CAST_H

/**
  @file
  @brief Data types for instance properties
*/



/**
  Copies value from `src` to `dest`.  If `dest_type` and `dest_size` differs
  from `src_type` and `src_size` the value will be casted, if possible.

  If `dest_type` contains allocated data, new memory will be allocated
  for `dest`.  Information may get lost in this case.

  Returns non-zero on error.
*/
int dlite_type_copy_cast(void *dest, DLiteType dest_type, size_t dest_size,
                         const void *src, DLiteType src_type, size_t src_size);



#endif /* _DLITE_TYPE_CAST_H */
