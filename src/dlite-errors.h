#ifndef _DLITE_ERRORS_H
#define _DLITE_ERRORS_H


/** DLite error codes */
typedef enum {
  /* Error codes copied form SWIG */
  dliteSuccess=0,         /*!< Success */
  dliteUnknownError=-1,   /*!< Generic unknown error */
  dliteIOError=-2,        /*!< File input/output error */
  dliteRuntimeError=-3,   /*!< Unspecified run-time error */
  dliteIndexError=-4,     /*!< Index out of range. Ex: int x[]={1,2,3}; x[7]; */
  dliteTypeError=-5,      /*!< Inappropriate argument type */
  dliteDivisionByZero=-6, /*!< Division by zero */
  dliteOverflowError=-7,  /*!< Result too large to be represented */
  dliteSyntaxError=-8,    /*!< Invalid syntax */
  dliteValueError=-9,     /*!< Inappropriate argument value */
  dliteSystemError=-10,   /*!< Internal error in DLite.  Please report this */
  dliteAttributeError=-11,/*!< Attribute or variable not found */
  dliteMemoryError=-12,   /*!< Out of memory */
  dliteNullReferenceError=-13,   /*!< Unexpected NULL argument */

  /* Additional DLite-specific errors */
  dliteOSError=-14,              /*!< Error calling a system function */
  dliteKeyError=-15,             /*!< Mapping key not found */
  dliteSearchError=-16,          /*!< Error in search */
  dliteParseError=-17,           /*!< Cannot parse input */
  dliteSerialiseError=-18,       /*!< Cannot serialise output */
  dliteUnsupportedError=-19,     /*!< Feature is not implemented/supported  */
  dliteVerifyError=-20,          /*!< Object cannot be verified */
  dliteInconsistentDataError=-21,/*!< Inconsistent data */
  dliteStorageOpenError=-22,     /*!< Cannot open storage plugin */
  dliteStorageLoadError=-23,     /*!< Cannot load storage plugin */
  dliteStorageSaveError=-24,     /*!< Cannot save storage plugin */
  dliteMissingInstanceError=-25, /*!< No instance with given id can be found */
  dliteMissingMetadataError=-26, /*!< No metadata with given id can be found */
  dliteMetadataExistError=-27,   /*!< Metadata with given id already exists */
  dlitePythonError=-28,          /*!< Error calling Python API */

  /* Should always be the last error */
  dliteLastError=-29
} DLiteErrCode;


/**
  Returns the name corresponding to error code
 */
const char *dlite_errname(DLiteErrCode code);


/**
  Returns a description of the corresponding to error code
 */
const char *dlite_errdescr(DLiteErrCode code);


/**
  Return DLite error code corresponding to `name`.  Unknown names will
  return `dliteUnknownError`.
 */
DLiteErrCode dlite_errcode(const char *name);


#endif  /* _DLITE_ERRORS_H */
