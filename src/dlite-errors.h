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
  dliteAttributeError=-11,/*!< Attribute not found */
  dliteMemoryError=-12,   /*!< Out of memory */
  dliteNullReferenceError=-13,   /*!< Unexpected NULL argument */

  /* Additional DLite-specific errors */
  dliteKeyError=-14,             /*!< Mapping key not found */
  dliteParseError=-15,           /*!< Cannot parse input */
  dlitePrintError=-16,           /*!< Cannot print format string */
  dliteUnsupportedError=-17,     /*!< Feature is not implemented/supported  */
  dliteInconsistentDataError=-18,/*!< Inconsistent data */
  dliteStorageOpenError=-19,     /*!< Cannot open storage plugin */
  dliteStorageLoadError=-20,     /*!< Cannot load storage plugin */
  dliteStorageSaveError=-21,     /*!< Cannot save storage plugin */
  dliteMissingInstanceError=-22, /*!< No instance with given id can be found */
  dliteMissingMetadataError=-23, /*!< No metadata with given id can be found */
  dliteMetadataExistError=-24,   /*!< Metadata with given id already exists */

  /* Should always be the last error */
  dliteLastError=-25
} DLiteErrors;

#endif  /* _DLITE_ERRORS_H */
