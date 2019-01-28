# - Try to find Jansson
# Once done this will define
#
#  JANSSON_FOUND - system has Jansson
#  JANSSON_INCLUDE_DIRS - the Jansson include directory
#  JANSSON_LIBRARIES - link these to use Jansson
#  JANSSON_LIBRARY_DIR - Jansson library directory (lib files)
#  JANSSON_RUNTIME_DIR - Jansson runtime directory (dlls)
#
#  Copyright (c) 2011 Lee Hambley <lee.hambley@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
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

  message("-- JANSSON_ROOT=${JANSSON_ROOT}")

  get_filename_component(JANSSON_LIBRARY_DIR ${JANSSON_LIBRARY} DIRECTORY)
  message("-- JANSSON_LIBRARY_DIR=${JANSSON_LIBRARY_DIR}")

  if(EXISTS ${JANSSON_ROOT}/bin)
    set(JANSSON_RUNTIME_DIR ${JANSSON_ROOT}/bin)
  else()
    set(JANSSON_RUNTIME_DIR "")
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Jansson DEFAULT_MSG
    JANSSON_INCLUDE_DIRS
    JANSSON_LIBRARIES
    JANSSON_LIBRARY_DIR
    JANSSON_RUNTIME_DIR
    )

  # show the JANSSON_INCLUDE_DIRS and JANSSON_LIBRARIES variables only
  # in the advanced view
  mark_as_advanced(
    JANSSON_INCLUDE_DIRS
    JANSSON_LIBRARIES
    JANSSON_LIBRARY_DIR
    JANSSON_RUNTIME_DIR
    )

endif()
