#ifndef _DLITE_ERRORS_H
#define _DLITE_ERRORS_H

typedef enum _DLiteErrors {
  /* Generic errors */
  dliteSussess,
  dliteUnknownError,
  dliteRuntimeError,
  dliteIOError,
  dliteIndexError,
  dliteValueError,
  dliteSystemError,
  dliteMemoryError,
  dliteNullReferenceError,   /* */

  /* Plugin-related errors */

  /* Should always be the last error */
  dliteLastError
};




#endif  /* _DLITE_ERRORS_H */
