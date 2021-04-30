# Once done this will define
#
#  JANSSON_FOUND        - whether system has Jansson
#  JANSSON_INCLUDE_DIRS - the Jansson include directory
#  JANSSON_LIBRARIES    - link these to use Jansson
#  JANSSON_LIBRARY_DIR  - Jansson library directory (lib files)
#  JANSSON_RUNTIME_DIR  - Jansson runtime directory (dlls)
#

if(JANSSON_LIBRARIES AND JANSSON_INCLUDE_DIRS)
  # in cache already
  set(JANSSON_FOUND TRUE)
else()
  find_path(JANSSON_INCLUDE_DIR
    NAMES
      jansson.h
    PATHS
      ${JANSSON_ROOT}/include
      $ENV{JANSSON_ROOT}/include
      $ENV{HOME}/.local/include
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    )
  list(APPEND JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
  get_filename_component(JANSSON_ROOT ${JANSSON_INCLUDE_DIR} DIRECTORY)

  find_library(JANSSON_LIBRARY
    NAMES jansson
    PATHS ${JANSSON_ROOT}/lib
    )
  list(APPEND JANSSON_LIBRARIES ${JANSSON_LIBRARY})

find_library(JANSSON_LIBRARY
  NAMES jansson libjansson
  HINTS ${PC_JANSSON_LIBDIR} ${PC_JANSSON_LIBRARY_DIRS}
  )

if(PC_JANSSON_VERSION)
  set(JANSSON_VERSION_STRING ${PC_JANSSON_VERSION})
elseif(JANSSON_INCLUDE_DIR AND EXISTS "${JANSSON_INCLUDE_DIR}/jansson.h")
  set(regex_jansson_version
    "^#define[ \t]+JANSSON_VERSION[ \t]+\"([^\"]+)\".*")

  file(STRINGS
    "${JANSSON_INCLUDE_DIR}/jansson.h" jansson_version
    REGEX "${regex_jansson_version}")
  string(REGEX REPLACE "${regex_jansson_version}" "\\1"
    JANSSON_VERSION_STRING "${jansson_version}")
  unset(regex_jansson_version)
  unset(jansson_version)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(jansson
  REQUIRED_VARS JANSSON_LIBRARY JANSSON_INCLUDE_DIR
  VERSION_VAR JANSSON_VERSION_STRING)

if(JANSSON_FOUND)
  set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
  set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
endif()

mark_as_advanced(JANSSON_INCLUDE_DIR JANSSON_LIBRARY)
