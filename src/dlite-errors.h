#ifndef _DLITE_ERRORS_H
#define _DLITE_ERRORS_H

/** DLite error codes */

typedef enum {
  dliteSuccess = 0,
  dliteUnknownError = -1,
  dliteIOError = -2,
  dliteRuntimeError = -3, /* originally -1 */
  dliteIndexError = -4,      /*!< Index out of range. Ex: int x[3] = {1,2,3}; x[7]; */
  dliteTypeError = -5, /*!< Unsported operands for type(s). Ex: int x = 3;char y = 'a'; x/y; */
  dliteDivisionByZero = -6,
  dliteOverflowError = -7,
  dliteSyntaxError = -8,
  dliteValueError = -9, /*!< Value not of expected type. Ex: get_int('apple'); */
  dliteSystemError = -10, 
  dliteAttributeError = -11, /*!< Attribute or method of object/struct not defined.
                                Ex1: obj.b; (if b not defined)
                                Ex2: int x=10; x.append(6); */ 
  dliteMemoryError = -12,
  dliteNullReferenceError = -13,
  dliteStorageOpenError = -14,     /*!< cannot open storage plugin */
  dliteStorageLoadError = -15,     /*!< cannot load storage plugin */
  dliteStorageSaveError = -16,     /*!< cannot save storage plugin */
  dliteMissingInstanceError = -17,
  dliteMissingMetadataError = -18,
  dliteMetadataExistError = -19,
  dliteParseError = -20,
  dliteFormatError = -21,

  /* Should always be the last error */
  dliteLastError=-999,
} DLiteErrors;

#endif  /* _DLITE_ERRORS_H */
