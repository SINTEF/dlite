# - Sets some reasonable defaults
# Provides default values for some cache variables if they have not
# been specified as arguments to cmake.
#
# Cache variables:
#
#   CMAKE_BUILD_TYPE      : "Debug"
#   CMAKE_INSTALL_PREFIX  : "$ENV{HOME}/.local"
#

# Defaults
set(default_build_type     "Debug")
set(default_install_prefix "$ENV{HOME}/.local")


# CMAKE_BUILD_TYPE
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
    STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


# CMAKE_INSTALL_PREFIX
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  message(STATUS "Setting install prefix to '${default_install_prefix}' as none was specified.")
  set(CMAKE_INSTALL_PREFIX "${default_install_prefix}" CACHE
    PATH "Install path prefix, prepended onto install directories." FORCE)
endif()
