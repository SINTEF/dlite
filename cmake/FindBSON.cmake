# Read-Only variables:
#  BSON_FOUND - system has the BSON library
#  BSON_INCLUDE_DIRS - the BSON include directory
#  BSON_LIBRARIES - The libraries needed to use BSON
#  BSON_VERSION - This is set to $major.$minor.$revision$path (eg. 0.4.1)

if (UNIX)
  find_package(PkgConfig QUIET)
  pkg_check_modules(_BSON QUIET libbson-1.0)
endif ()

find_path(BSON_INCLUDE_DIRS
  NAMES
    libbson-1.0/bson.h
  HINTS
    ${BSON_ROOT_DIR}
    ${_BSON_INCLUDEDIR}
  PATH_SUFFIXES
    include
)

set(BSON_INCLUDE_DIRS "${BSON_INCLUDE_DIRS}/libbson-1.0")

if(WIN32 AND MSVC)
  find_library(BSON
    NAMES
      "bson-1.0"
    HINTS
      ${BSON_ROOT_DIR}
    PATH_SUFFIXES
      bin
      lib
  )
  mark_as_advanced(BSON)
  set(BSON_LIBRARIES ${BSON} ws2_32)

else()

  find_library(BSON_LIBRARY
    NAMES
      bson-1.0
    HINTS
      ${BSON_ROOT_DIR}
      ${_BSON_LIBDIR}
    PATH_SUFFIXES
      lib
  )
  mark_as_advanced(BSON_LIBRARY)

  find_package (Threads REQUIRED)

  set(BSON_LIBRARIES ${BSON_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

endif()

if (BSON_INCLUDE_DIRS)
  if (_BSON_VERSION)
     set(BSON_VERSION "${_BSON_VERSION}")
  elseif(BSON_INCLUDE_DIRS AND EXISTS "${BSON_INCLUDE_DIRS}/bson-version.h")
     file(STRINGS "${BSON_INCLUDE_DIRS}/bson-version.h" bson_version_str
        REGEX "^#define[\t ]+BSON_VERSION[\t ]+\([0-9.]+\)[\t ]+$")

     string(REGEX REPLACE "^.*BSON_VERSION[\t ]+\([0-9.]+\)[\t ]+$"
        "\\1" BSON_VERSION "${bson_version_str}")
  endif ()
endif ()

include(FindPackageHandleStandardArgs)

if (BSON_VERSION)
   find_package_handle_standard_args(BSON
    REQUIRED_VARS
      BSON_LIBRARIES
      BSON_INCLUDE_DIRS
    VERSION_VAR
      BSON_VERSION
    FAIL_MESSAGE
      "Could NOT find BSON version"
  )
else ()
   find_package_handle_standard_args(BSON DEFAULT_MSG
      BSON_LIBRARIES
      BSON_INCLUDE_DIRS
  )
endif ()

mark_as_advanced(BSON_INCLUDE_DIRS BSON_LIBRARIES)
