# - Try to find uuid
# Once done this will define
#
#  UUID_FOUND - system has uuid
#  UUID_INCLUDE_DIRS - the uuid include directory
#  uuid - shared library target
#  uuid-static - static library target
#

if(UUID_INCLUDE_DIRS)
  # in cache already
  set(UUID_FOUND TRUE)
else()
  find_path(UUID_INCLUDE_DIR
    NAMES
      uuid/uuid.h
    PATHS
      ${UUID_ROOT}/include
      $ENV{HOME}/.local/include
      /usr/include
      /usr/local/include
    )
  list(APPEND UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})

  find_library(UUID_LIBRARY
    NAMES
      uuid
    PATHS
      ${UUID_ROOT}/lib64
      ${UUID_ROOT}/lib
      $ENV{HOME}/.local/lib64
      $ENV{HOME}/.local/lib
      /usr/lib64
      /usr/lib
      /usr/local/lib64
      /usr/local/lib
    )
  list(APPEND UUID_LIBRARIES ${UUID_LIBRARY})

  find_library(UUID_STATIC_LIBRARY
    NAMES
      uuid-static
    PATHS
      ${UUID_ROOT}/lib
      ${UUID_ROOT}/lib64
      /usr/lib
      /usr/lib64
      /usr/local/lib
      /usr/local/lib64
      $ENV{HOME}/.local/lib
      $ENV{HOME}/.local/lib64
    )
  list(APPEND UUID_STATIC_LIBRARIES ${UUID_STATIC_LIBRARY})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(uuid DEFAULT_MSG
    UUID_LIBRARIES UUID_INCLUDE_DIRS)

  # show the UUID_INCLUDE_DIRS and UUID_LIBRARIES variables only in
  # the advanced view
  mark_as_advanced(UUID_INCLUDE_DIRS UUID_LIBRARIES UUID_STATIC_LIBRARIES)

endif()
