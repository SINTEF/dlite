# - Try to find dlite
#
# Once done this will define:
#
#   DLITE_FOUND - system has DLITE
#   DLITE_INCLUDE_DIRS - the DLITE include directory
#   DLITE_LIBRARIES - Link these to use DLITE
#   DLITE_LIBRARY_DIR - Add to link_directories
#   DLITE_TEMPLATE_DIR - Directory where templates are stored
#   DLITE_ROOT - Link to root/install directory
#
# DLITE_ROOT may be set at call time
#
#

if (DLITE_LIBRARIES AND DLITE_INCLUDE_DIRS)
  # in cache already
  set(DLITE_FOUND TRUE)
else (DLITE_LIBRARIES AND DLITE_INCLUDE_DIRS)
  find_path(DLITE_INCLUDE_DIR
    NAMES
      dlite.h
    PATHS
      /usr/include/dlite
      /usr/local/include/dlite
      /opt/local/include/dlite
      /sw/include/dlite
      $ENV{HOME}/.local/include/dlite
      ${DLITE_ROOT}/include/dlite
  )

  find_library(DLITE_LIBRARY
    NAMES
      dlite
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
      $ENV{HOME}/.local/lib
      ${DLITE_ROOT}/lib
  )

  find_path(DLITE_BINARY_DIR
    NAMES
      dlite-codegen.exe
    PATHS
      /usr/bin
      /usr/local/bin
      /opt/local/bin
      /sw/bin
      $ENV{HOME}/.local/bin
      ${DLITE_ROOT}/bin
  )

  set(DLITE_INCLUDE_DIRS
    ${DLITE_INCLUDE_DIR}
  )

  if (DLITE_LIBRARY)
    set(DLITE_LIBRARIES
      ${DLITE_LIBRARY}
      ${DLITE_LIBRARIES}
      )
  endif (DLITE_LIBRARY)

  get_filename_component(DLITE_LIBRARY_DIR ${DLITE_LIBRARY} DIRECTORY)

  find_package(HDF5 COMPONENTS C)

  find_library(JANSSON_LIBRARY
	NAMES
	  jansson
	PATHS
	  /usr/lib
	  /usr/local/lib
	  /opt/local/lib
	  /sw/lib
	  ${JANSSON_ROOT}/lib
  )
  if (JANSSON_LIBRARY)
    set(JANSSON_LIBRARIES
      ${JANSSON_LIBRARIES}
      ${JANSSON_LIBRARY}
      )
  endif (JANSSON_LIBRARY)

  set(DLITE_LIBRARIES ${DLITE_LIBRARIES}
    dlite-plugins-json  # FIXME - remove when plugins are implemented
    dlite-plugins-hdf5  # FIXME - remove when plugins are implemented
    dlite-utils
    uuid
    ${JANSSON_LIBRARIES}
    ${HDF5_LIBRARIES}
    )

  get_filename_component(DLITE_ROOT ${DLITE_LIBRARY_DIR} DIRECTORY)
  set(DLITE_TEMPLATE_DIR ${DLITE_ROOT}/share/dlite/templates)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(DLITE DEFAULT_MSG
    DLITE_LIBRARIES
    DLITE_INCLUDE_DIRS
    DLITE_LIBRARY_DIR
    DLITE_TEMPLATE_DIR
    DLITE_ROOT
    )

  # show the DLITE_INCLUDE_DIRS and DLITE_LIBRARIES variables only in the advanced view
  mark_as_advanced(
    DLITE_INCLUDE_DIRS
    DLITE_LIBRARIES
    DLITE_LIBRARY_DIR
    DLITE_TEMPLATE_DIR
    DLITE_ROOT
    )

endif (DLITE_LIBRARIES AND DLITE_INCLUDE_DIRS)
