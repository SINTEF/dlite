#ifndef _DLITE_ERRORS_H
#define _DLITE_ERRORS_H


/** DLite error codes */
typedef enum {
  /* Error codes copied form SWIG */
  dliteSuccess=0,              /*!< Success */
  dliteUnknownError=-1,        /*!< Generic unknown error */
  dliteIOError=-2,             /*!< File input/output error */
  dliteRuntimeError=-3,        /*!< Unspecified run-time error */
  dliteIndexError=-4,          /*!< Index out of range. Ex: int x[]={1,2,3}; x[7]; */
  dliteTypeError=-5,           /*!< Inappropriate argument type */
  dliteDivisionByZeroError=-6, /*!< Division by zero */
  dliteOverflowError=-7,       /*!< Result too large to be represented */
  dliteSyntaxError=-8,         /*!< Invalid syntax */
  dliteValueError=-9,          /*!< Inappropriate argument value */
  dliteSystemError=-10,        /*!< Internal error in DLite.  Please report this */
  dliteAttributeError=-11,     /*!< Attribute or variable not found */
  dliteMemoryError=-12,        /*!< Out of memory */
  dliteNullReferenceError=-13, /*!< Unexpected NULL argument */

  /* Additional DLite-specific errors */
  dliteOSError=-14,              /*!< Error calling a system function */
  dliteKeyError=-15,             /*!< Mapping key not found */
  dliteNameError=-16,            /*!< Name not found */
  dliteLookupError=-17,          /*!< Error looking up item */
  dliteParseError=-18,           /*!< Cannot parse input */
  dlitePermissionError=-19,      /*!< Not enough permissions */
  dliteSerialiseError=-20,       /*!< Cannot serialise output */
  dliteUnsupportedError=-21,     /*!< Feature is not implemented/supported  */
  dliteVerifyError=-22,          /*!< Object cannot be verified */
  dliteInconsistentDataError=-23,/*!< Inconsistent data */
  dliteInvalidMetadataError=-24, /*!< Invalid metadata */
  dliteStorageOpenError=-25,     /*!< Cannot open storage plugin */
  dliteStorageLoadError=-26,     /*!< Cannot load storage plugin */
  dliteStorageSaveError=-27,     /*!< Cannot save storage plugin */
  dliteOptionError=-28,          /*!< Invalid storage plugin option */
  dliteMissingInstanceError=-29, /*!< No instance with given id can be found */
  dliteMissingMetadataError=-30, /*!< No metadata with given id can be found */
  dliteMetadataExistError=-31,   /*!< Metadata with given id already exists */
  dliteMappingError=-32,         /*!< Error in instance mappings */
  dlitePythonError=-33,          /*!< Error calling Python API */

  /* Should always be the last error */
  dliteLastError=-34
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
