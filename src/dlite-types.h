#ifndef _DLITE_TYPES_H
#define _DLITE_TYPES_H

/**
  @file
*/

/** Basic data types */
typedef enum _DLiteType {
  dliteBlob,           /*!< Binary blob */
  dliteBool,           /*!< Boolean */
  dliteInt,            /*!< Signed integer */
  dliteUInt,           /*!< Unigned integer */
  dliteFloat,          /*!< Floating point */
  dliteString,         /*!< Fix-sized string */
  dliteStringPtr       /*!< Pointer to NUL-terminated string */
} DLiteType;


#endif /* _DLITE_TYPES_H */
