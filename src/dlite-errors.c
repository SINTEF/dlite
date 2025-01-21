#include <stdlib.h>
#include <string.h>

#include "dlite-errors.h"


/*
  Returns the name corresponding to error code (with the final "Error" stripped
  off).
 */
const char *dlite_errname(DLiteErrCode code)
{
  switch (code) {
  case dliteSuccess:                return "DLiteSuccess";
  case dliteUnknownError:           return "DLiteUnknown";
  case dliteIOError:                return "DLiteIO";
  case dliteRuntimeError:           return "DLiteRuntime";
  case dliteIndexError:             return "DLiteIndex";
  case dliteTypeError:              return "DLiteType";
  case dliteDivisionByZeroError:    return "DLiteDivisionByZero";
  case dliteOverflowError:          return "DLiteOverflow";
  case dliteSyntaxError:            return "DLiteSyntax";
  case dliteValueError:             return "DLiteValue";
  case dliteSystemError:            return "DLiteSystem";
  case dliteAttributeError:         return "DLiteAttribute";
  case dliteMemoryError:            return "DLiteMemory";
  case dliteNullReferenceError:     return "DLiteNullReference";

  case dliteOSError:                return "DLiteOS";
  case dliteKeyError:               return "DLiteKey";
  case dliteNameError:              return "DLiteName";
  case dliteLookupError:            return "DLiteLookup";
  case dliteParseError:             return "DLiteParse";
  case dlitePermissionError:        return "DLitePermission";
  case dliteSerialiseError:         return "DLiteSerialise";
  case dliteUnsupportedError:       return "DLiteUnsupported";
  case dliteVerifyError:            return "DLiteVerify";
  case dliteInconsistentDataError:  return "DLiteInconsistentData";
  case dliteInvalidMetadataError:   return "DLiteInvalidMetadata";
  case dliteStorageOpenError:       return "DLiteStorageOpen";
  case dliteStorageLoadError:       return "DLiteStorageLoad";
  case dliteStorageSaveError:       return "DLiteStorageSave";
  case dliteOptionError:            return "DLiteOption";
  case dliteMissingInstanceError:   return "DLiteMissingInstance";
  case dliteMissingMetadataError:   return "DLiteMissingMetadata";
  case dliteMetadataExistError:     return "DLiteMetadataExist";
  case dliteMappingError:           return "DLiteMapping";
  case dliteProtocolError:          return "DLiteProtocol";
  case dlitePythonError:            return "DLitePython";
  case dliteTimeoutError:           return "DLiteTimeout";

  case dliteLastError:              return "DLiteUndefined";
  }
  if (code < 0) return "DLiteUndefined";
  return "DLiteOther";
}


/*
  Returns a description of the corresponding to error code
 */
const char *dlite_errdescr(DLiteErrCode code)
{
  switch (code) {
  case dliteSuccess:                return "Success";
  case dliteUnknownError:           return "Generic unknown error";
  case dliteIOError:                return "I/O related error";
  case dliteRuntimeError:           return "Unspecified run-time error";
  case dliteIndexError:             return "Index out of range";
  case dliteTypeError:              return "Inappropriate argument type";
  case dliteDivisionByZeroError:    return "Division by zero";
  case dliteOverflowError:          return "Result too large to be represented";
  case dliteSyntaxError:            return "Invalid syntax";
  case dliteValueError:             return "Inappropriate argument value (of correct type)";
  case dliteSystemError:            return "Internal error in DLite.  Please report this";
  case dliteAttributeError:         return "Cannot refer to or assign attribute or variable";
  case dliteMemoryError:            return "Out of memory";
  case dliteNullReferenceError:     return "Unexpected NULL pointer when converting bindings";

  case dliteOSError:                return "Error calling a system function";
  case dliteKeyError:               return "Mapping key is not found";
  case dliteNameError:              return "Name not found";
  case dliteLookupError:            return "Error looking up item";
  case dliteParseError:             return "Cannot parse input";
  case dlitePermissionError:        return "Not enough permissions";
  case dliteSerialiseError:         return "Cannot serialise output";
  case dliteUnsupportedError:       return "Feature is not implemented/supported";
  case dliteVerifyError:            return "Object cannot be verified";
  case dliteInconsistentDataError:  return "Inconsistent data";
  case dliteInvalidMetadataError:   return "Invalid metadata";
  case dliteStorageOpenError:       return "Cannot open storage plugin";
  case dliteStorageLoadError:       return "Cannot load storage plugin";
  case dliteStorageSaveError:       return "Cannot save storage plugin";
  case dliteOptionError:            return "Invalid storage plugin option";
  case dliteMissingInstanceError:   return "No instance with given id";
  case dliteMissingMetadataError:   return "No metadata with given id";
  case dliteMetadataExistError:     return "Metadata with given id already exists";
  case dliteMappingError:           return "Error in instance mappings";
  case dliteProtocolError:          return "Error in a protocol plugin";
  case dlitePythonError:            return "Error calling Python API";
  case dliteTimeoutError:           return "Raised when a function times out";
  case dliteLastError:              return NULL;
  }
  return NULL;
}


/*
  Return DLite error code corresponding to `name`.

  Special cases:
    - Unknown names will return `dliteUnknownError`.
    - "DLiteError" will return zero.
 */
DLiteErrCode dlite_errcode(const char *name)
{
  DLiteErrCode code;
  if (strncmp("DLiteError", name, 10) == 0) return 0;
  for (code=0; code>dliteLastError; code--) {
    const char *errname = dlite_errname(code);
    if (strncmp(errname, name, strlen(errname)) == 0)
      return code;
  }
  return dliteUnknownError;
}
