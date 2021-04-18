# - Try to find the Rasqal rdf query library (http://librdf.org/rasqal/)
# Once done this will define
#
#  RASQAL_FOUND       - system has Rasqal
#  RASQAL_LIBRARIES   - Link these to use RASQAL
#  RASQAL_INCLUDE_DIR - The include directory for using rasqal
#  RASQAL_DEFINITIONS - Compiler switches required for using RASQAL
#  RASQAL_VERSION     - The rasqal version string


if(RASQAL_INCLUDE_DIR AND RASQAL_LIBRARIES)
  # in cache already
  set(RASQAL_FOUND TRUE)
else()

  find_program(
    RASQAL_CONFIG
    NAMES rasqal-config
    )

  if(RASQAL_CONFIG)
    execute_process(
      COMMAND ${RASQAL_CONFIG} --version
      OUTPUT_VARIABLE RASQAL_VERSION
      )
    if(RASQAL_VERSION)
      string(REPLACE "\n" "" RASQAL_VERSION ${RASQAL_VERSION})

      # extract include paths from rasqal-config
      execute_process(
        COMMAND ${RASQAL_CONFIG} --cflags
        OUTPUT_VARIABLE rasqal_CFLAGS_ARGS)
      string( REPLACE " " ";" rasqal_CFLAGS_ARGS ${rasqal_CFLAGS_ARGS} )
      foreach(_ARG ${rasqal_CFLAGS_ARGS})
        if(${_ARG} MATCHES "^-I")
          string(REGEX REPLACE "^-I" "" _ARG ${_ARG})
          string( REPLACE "\n" "" _ARG ${_ARG} )
          list(APPEND rasqal_INCLUDE_DIRS ${_ARG})
        endif(${_ARG} MATCHES "^-I")
      endforeach(_ARG)

      # extract lib paths from rasqal-config
      execute_process(
        COMMAND ${RASQAL_CONFIG} --libs
        OUTPUT_VARIABLE rasqal_CFLAGS_ARGS)
      string( REPLACE " " ";" rasqal_CFLAGS_ARGS ${rasqal_CFLAGS_ARGS} )
      foreach(_ARG ${rasqal_CFLAGS_ARGS})
        if(${_ARG} MATCHES "^-L")
          string(REGEX REPLACE "^-L" "" _ARG ${_ARG})
          list(APPEND rasqal_LIBRARY_DIRS ${_ARG})
        endif(${_ARG} MATCHES "^-L")
      endforeach(_ARG)
    endif(RASQAL_VERSION)
  endif(RASQAL_CONFIG)

  find_path(RASQAL_INCLUDE_DIR rasqal.h
    PATHS
      ${redland_INCLUDE_DIRS}
      ${rasqal_INCLUDE_DIRS}
      /usr/X11/include
    PATH_SUFFIXES
      redland rasqal
    )

  find_library(RASQAL_LIBRARIES NAMES rasqal librasqal
    PATHS
      ${rasqal_LIBRARY_DIRS}
    )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(
    Rasqal
    VERSION_VAR   RASQAL_VERSION
    REQUIRED_VARS RASQAL_LIBRARIES RASQAL_INCLUDE_DIR)

  if (RASQAL_FOUND)
    set(RASQAL_DEFINITIONS ${rasqal_CFLAGS})
    if (NOT Rasqal_FIND_QUIETLY)
      message(STATUS "Found Rasqal ${RASQAL_VERSION}: libs - ${RASQAL_LIBRARIES}; includes - ${RASQAL_INCLUDE_DIR}")
    endif (NOT Rasqal_FIND_QUIETLY)
  else (RASQAL_FOUND)
    if (Rasqal_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find Rasqal")
    endif (Rasqal_FIND_REQUIRED)
  endif (RASQAL_FOUND)


  mark_as_advanced(
    RASQAL_INCLUDE_DIR_TMP
    RASQAL_INCLUDE_DIR
    RASQAL_LIBRARIES)

endif()
