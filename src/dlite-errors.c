#include "dlite-errors.h"


/*
  Returns the name corresponding to error code
 */
const char *dlite_errname(DLiteErrors code)
{
  switch (code) {
  case dliteSuccess:                return "DLiteSussess";
  case dliteUnknownError:           return "DLiteUnknown";
  case dliteIOError:                return "DLiteIO";
  case dliteRuntimeError:           return "DLiteRuntime";
  case dliteIndexError:             return "DLiteIndex";
  case dliteTypeError:              return "DLiteType";
  case dliteDivisionByZero:         return "DLiteDivisionByZero";
  case dliteOverflowError:          return "DLiteOverflow";
  case dliteSyntaxError:            return "DLiteSyntax";
  case dliteValueError:             return "DLiteValue";
  case dliteSystemError:            return "DLiteSystem";
  case dliteAttributeError:         return "DLiteAttribute";
  case dliteMemoryError:            return "DLiteMemory";
  case dliteNullReferenceError:     return "DLiteNullReference";

  case dliteKeyError:               return "DLiteKey";
  case dliteParseError:             return "DLiteParse";
  case dlitePrintError:             return "DLitePrint";
  case dliteUnsupportedError:       return "DLiteUnsupported";
  case dliteInconsistentDataError:  return "DLiteInconsistentData";
  case dliteStorageOpenError:       return "DLiteStorageOpen";
  case dliteStorageLoadError:       return "DLiteStorageLoad";
  case dliteStorageSaveError:       return "DLiteStorageSave";
  case dliteMissingInstanceError:   return "DLiteMissingInstance";
  case dliteMissingMetadataError:   return "DLiteMissingMetadata";
  case dliteMetadataExistError:     return "DLiteMetadataExist";

  case dliteLastError:              return "DLiteLast";
  }
  if (code < 0) return "DLiteUndefined";
  return "DLiteOther";
}
