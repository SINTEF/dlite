# - Try to find Redland
# Once done this will define
#
#  REDLAND_FOUND        - system has Redland
#  REDLAND_INCLUDE_DIRS - the Redland include directory
#  REDLAND_LIBRARIES    - link these to use Redland
#  REDLAND_LIBRARY_DIR  - Redland library directory (lib files)
#  REDLAND_RUNTIME_DIR  - Redland runtime directory (dlls)
#

if(REDLAND_LIBRARIES AND REDLAND_INCLUDE_DIRS)
  # in cache already
  set(REDLAND_FOUND TRUE)
else()

  find_path(REDLAND_INCLUDE_DIR
    NAMES
      redland.h
    PATHS
      ${REDLAND_ROOT}/include
      $ENV{REDLAND_ROOT}/include
      $ENV{HOME}/.local/include
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    )
  list(APPEND REDLAND_INCLUDE_DIRS ${REDLAND_INCLUDE_DIR})

  get_filename_component(REDLAND_ROOT ${REDLAND_INCLUDE_DIR} DIRECTORY)

  find_library(REDLAND_LIBRARY
    NAMES rdf
    PATHS ${REDLAND_ROOT}/lib
    )
  list(APPEND REDLAND_LIBRARIES ${REDLAND_LIBRARY})

  get_filename_component(REDLAND_LIBRARY_DIR ${REDLAND_LIBRARY} DIRECTORY)

  if(EXISTS ${REDLAND_ROOT}/bin)
    set(REDLAND_RUNTIME_DIR ${REDLAND_ROOT}/bin)
  else()
    set(REDLAND_RUNTIME_DIR "")
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Redland DEFAULT_MSG
    REDLAND_INCLUDE_DIRS
    REDLAND_LIBRARIES
    REDLAND_LIBRARY_DIR
    REDLAND_RUNTIME_DIR
    )

  # show the REDLAND_INCLUDE_DIRS and REDLAND_LIBRARIES variables only
  # in the advanced view
  mark_as_advanced(
    REDLAND_INCLUDE_DIRS
    REDLAND_LIBRARIES
    REDLAND_LIBRARY_DIR
    REDLAND_RUNTIME_DIR
    )

endif()
