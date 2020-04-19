#ifndef _DLITE_ERRORS_H
#define _DLITE_ERRORS_H

/** DLite error codes */
typedef enum {
  dliteRuntimeError = -1,
  dliteSussess = 0,
  dliteUnknownError,
  dliteIOError,
  dliteIndexError,
  dliteValueError,
  dliteSystemError,
  dliteMemoryError,
  dliteNullReferenceError,
  dliteStorageOpenError,     /*!< cannot open storage plugin */
  dliteStorageLoadError,     /*!< cannot load storage plugin */

  /* Should always be the last error */
  dliteLastError
} DLiteErrors;


#endif  /* _DLITE_ERRORS_H */
