#include "dlite-errors.h"


/*
  Returns the name corresponding to error code
 */
const char *dlite_errname(DLiteErrors code)
{
  switch (code) {
  case dliteSuccess:                return "Sussess";
  case dliteUnknownError:           return "UnknownError";
  case dliteIOError:                return "IOError";
  case dliteRuntimeError:           return "RuntimeError";
  case dliteIndexError:             return "IndexError";
  case dliteTypeError:              return "TypeError";
  case dliteDivisionByZero:         return "DivisionByZero";
  case dliteOverflowError:          return "OverflowError";
  case dliteSyntaxError:            return "SyntaxError";
  case dliteValueError:             return "ValueError";
  case dliteSystemError:            return "SystemError";
  case dliteAttributeError:         return "AttributeError";
  case dliteMemoryError:            return "MemoryError";
  case dliteNullReferenceError:     return "NullReferenceError";

  case dliteKeyError:               return "KeyError";
  case dliteParseError:             return "ParseError";
  case dlitePrintError:             return "PrintError";
  case dliteUnsupportedError:       return "UnsupportedError";
  case dliteInconsistentDataError:  return "InconsistentDataError";
  case dliteStorageOpenError:       return "StorageOpenError";
  case dliteStorageLoadError:       return "StorageLoadError";
  case dliteStorageSaveError:       return "StorageSaveError";
  case dliteMissingInstanceError:   return "MissingInstanceError";
  case dliteMissingMetadataError:   return "MissingMetadataError";
  case dliteMetadataExistError:     return "MetadataExistError";

  case dliteLastError:              return "LastError";
  }
  if (code < 0) return "UndefinedError";
  return "Successful";
}
