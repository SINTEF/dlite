# - Try to find dlite
#
# Once done this will define:
#
#   DLITE_FOUND - system has DLite
#   DLITE_INCLUDE_DIRS - the DLite include directory
#   DLITE_LIBRARIES - Link these to use DLite
#   DLITE_LIBRARY_DIR - Add to link_directories
#   DLITE_RUNTIME_DIR - Directory with executables (and DLLs on Windows)
#   DLITE_TEMPLATE_DIR - Directory where templates are stored
#   DLITE_STORAGE_PLUGINS - Directory path for storage plugins
#   DLITE_MAPPINGS_PLUGINS - Directory path for mappings plugins
#   DLITE_PYTHON_STORAGES - Directory path for Python storage plugins
#   DLITE_PYTHON_MAPPINGS - Directory path for Python mapping plugins
#   DLITE_ROOT - Root of install directory
#
# Additionally the following variables will be defined, used for setting
# up the environment for DLite
#
#   dlite_PATH - path to dlite DLite executables
#   dlite_LD_LIBRARY_PATH - path to dlite libraries
#   dlite_STORAGE_PLUGINS - path to storage plugins
#   dlite_MAPPINGS_PLUGINS - path to storage plugins
#
# Att call time, the following may be set to modify the behaviour of this
# module:
#
#   DLITE_ROOT - look for dlite at this location
#   DLITE_REQUIRED - if true, it will be an error if DLite is not found
#

if(DLITE_LIBRARIES AND DLITE_INCLUDE_DIRS)
  # in cache already
  set(DLITE_FOUND TRUE)
else()

  # DLITE_INCLUDE_DIRS
  find_path(DLITE_INCLUDE_DIR
    NAMES
      dlite.h
    PATHS
      ${DLITE_ROOT}/include/dlite
      $ENV{HOME}/.local/include/dlite
      /usr/include/dlite
      /usr/local/include/dlite
      /opt/local/include/dlite
      /sw/include/dlite
    )
  list(APPEND DLITE_INCLUDE_DIRS ${DLITE_INCLUDE_DIR})

  # DLITE_ROOT
  get_filename_component(INCLUDES ${DLITE_INCLUDE_DIR} DIRECTORY)
  get_filename_component(DLITE_ROOT "${INCLUDES}" DIRECTORY)

  if(DLITE_ROOT)
    message(STATUS "Found DLite: ${DLITE_ROOT}")
  elseif(DLITE_REQUIRED)
    message(FATAL_ERROR "DLite not found (try to specify DLITE_ROOT)")
  else()
    message(STATUS "DLite not found")
  endif()

  # DLITE_LIBRARY_DIR
  set(DLITE_LIBRARY_DIR ${DLITE_ROOT}/lib)

  # DLITE_RUNTIME_DIR
  set(DLITE_RUNTIME_DIR ${DLITE_ROOT}/bin)

  # DLITE_TEMPLATE_DIR
  set(DLITE_TEMPLATE_DIR ${DLITE_ROOT}/share/dlite/templates)

  # DLITE_STORAGE_PLUGINS
  set(DLITE_STORAGE_PLUGINS ${DLITE_ROOT}/share/dlite/storages)

  # DLITE_MAPPINGS_PLUGINS
  set(DLITE_MAPPINGS_PLUGINS ${DLITE_ROOT}/share/dlite/mappings)

  # DLITE_PYTHON_STORAGES
  set(DLITE_PYTHON_STORAGES ${DLITE_ROOT}/share/dlite/python-storages)

  # DLITE_PYTHON_MAPPINGS
  set(DLITE_PYTHON_MAPPINGS ${DLITE_ROOT}/share/dlite/python-mappings)

  # DLITE_LIBRARIES
  find_library(DLITE_LIBRARY
    NAMES dlite
    PATHS ${DLITE_LIBRARY_DIR}
    )
  find_library(DLITE_UTILS_LIBRARY
    NAMES dlite-utils
    PATHS ${DLITE_LIBRARY_DIR}
    )
  list(APPEND DLITE_LIBRARIES ${DLITE_LIBRARY} ${DLITE_UTILS_LIBRARY})

  # Find HDF5 and Jansson
  # FIXME - when dlite is using dynamic loaded plugins we should:
  #     Windows: append Jansson and HDF5 runtime library paths to dlite_PATH
  #     Linux:   append Jansson and HDF5 library paths to dlite_LD_LIBRARY_PATH
  find_library(JANSSON_LIBRARY
    NAMES
      jansson
    PATHS
      ${JANSSON_ROOT}/lib
      $ENV{HOME}/.local/lib
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
    )
  list(APPEND JANSSON_LIBRARIES ${JANSSON_LIBRARY})

  find_package(HDF5 COMPONENTS C)

  list(APPEND DLITE_LIBRARIES
    ${JANSSON_LIBRARIES}
    ${HDF5_LIBRARIES}
    )


  # Define variables for setting up environment needed by dlite
  if(WINDOWS)
    set(dlite_PATH
      "${DLITE_RUNTIME_DIR};${DLITE_LIBRARY_DIR};$ENV{PATH}")
    set(dlite_LD_LIBRARY_PATH
      "$ENV{LD_LIBRARY_PATH}")
    set(dlite_STORAGE_PLUGINS
      "${DLITE_STORAGE_PLUGINS};$ENV{DLITE_STORAGE_PLUGINS}")
    set(dlite_MAPPINGS_PLUGINS
      "${DLITE_MAPPINGS_PLUGINS};$ENV{DLITE_MAPPINGS_PLUGINS}")
  else()
    set(dlite_PATH
      "${DLITE_RUNTIME_DIR}:$ENV{PATH}")
    set(dlite_LD_LIBRARY_PATH
      "${DLITE_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}")
    set(dlite_STORAGE_PLUGINS
      "${DLITE_STORAGE_PLUGINS}:$ENV{DLITE_STORAGE_PLUGINS}")
    set(dlite_MAPPINGS_PLUGINS
      "${DLITE_MAPPINGS_PLUGINS}:$ENV{DLITE_MAPPINGS_PLUGINS}")
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(DLITE DEFAULT_MSG
    DLITE_INCLUDE_DIRS
    DLITE_LIBRARIES
    DLITE_LIBRARY_DIR
    DLITE_RUNTIME_DIR
    DLITE_TEMPLATE_DIR
    DLITE_STORAGE_PLUGINS
    DLITE_MAPPINGS_PLUGINS
    DLITE_ROOT

    dlite_PATH
    dlite_LD_LIBRARY_PATH
    dlite_STORAGE_PLUGINS
    dlite_MAPPINGS_PLUGINS
    )

  # Show the DLITE_INCLUDE_DIRS and DLITE_LIBRARIES variables only in
  # the advanced view
  mark_as_advanced(
    DLITE_INCLUDE_DIRS
    DLITE_LIBRARIES
    DLITE_LIBRARY_DIR
    DLITE_RUNTIME_DIR
    DLITE_TEMPLATE_DIR
    DLITE_STORAGE_PLUGINS
    DLITE_MAPPINGS_PLUGINS
    DLITE_ROOT

    dlite_PATH
    dlite_LD_LIBRARY_PATH
    dlite_STORAGE_PLUGINS
    dlite_MAPPINGS_PLUGINS
    )

endif()
