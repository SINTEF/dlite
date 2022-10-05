#ifndef _DLITE_ERRORS_H
#define _DLITE_ERRORS_H

/** DLite error codes */

typedef enum {
  dliteSuccess = 0,
  dliteUnknownError = -1,
  dliteIOError = -2,
  dliteRuntimeError = -3, /* originally -1 */
  dliteIndexError = -4,      /*!< Index out of range. Ex: int x[3] = {1,2,3}; x[7]; */
  dliteTypeError = -5, // e.g. float / str
  dliteDivisionByZero = -6,
  dliteOverflowError = -7,
  dliteSyntaxError = -8,
  dliteValueError = -9, // e.g. int('apple')
  dliteSystemError = -10, 
  dliteAttributeError = -11, // e.g. obj.b (if b not defined) x=10, x.append(6)
  dliteMemoryError = -12,
  dliteNullReferenceError = -13,
  dliteStorageOpenError = -14,     /*!< cannot open storage plugin */
  dliteStorageLoadError = -15,     /*!< cannot load storage plugin */
  dliteStorageSaveError = -16,     /*!< cannot save storage plugin */

  /* Should always be the last error */
  dliteLastError=-17,
} DLiteErrors;

#endif  /* _DLITE_ERRORS_H */
