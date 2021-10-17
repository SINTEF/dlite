#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dlite-utils" for configuration "Release"
set_property(TARGET dlite-utils APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dlite-utils PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/dlite-utils.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/dlite-utils.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS dlite-utils )
list(APPEND _IMPORT_CHECK_FILES_FOR_dlite-utils "${_IMPORT_PREFIX}/lib/dlite-utils.lib" "${_IMPORT_PREFIX}/bin/dlite-utils.dll" )

# Import target "dlite-utils-static" for configuration "Release"
set_property(TARGET dlite-utils-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dlite-utils-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/dlite-utils-static.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS dlite-utils-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_dlite-utils-static "${_IMPORT_PREFIX}/lib/dlite-utils-static.lib" )

# Import target "dlite" for configuration "Release"
set_property(TARGET dlite APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dlite PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/dlite.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/dlite.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS dlite )
list(APPEND _IMPORT_CHECK_FILES_FOR_dlite "${_IMPORT_PREFIX}/lib/dlite.lib" "${_IMPORT_PREFIX}/bin/dlite.dll" )

# Import target "dlite-static" for configuration "Release"
set_property(TARGET dlite-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dlite-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/dlite-static.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS dlite-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_dlite-static "${_IMPORT_PREFIX}/lib/dlite-static.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
