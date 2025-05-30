# -*- Mode: cmake -*-
project(dlite-tools C)

# TODO - It would be preferable to link with the shared libraries.
# How to do that without breaking the docker build?

if(MSVC)  # Unset extra_link_libraries on MSVC to avoid linking against None.lib
  unset(extra_link_libraries)
endif()

# dlite-getuuid
add_executable(dlite-getuuid dlite-getuuid.c)
target_link_libraries(dlite-getuuid
  dlite-static
  dlite-utils-static
  ${extra_link_libraries}
)
target_include_directories(dlite-getuuid PRIVATE
  ${dlite_SOURCE_DIR}/src
)


# dlite-codegen
add_executable(dlite-codegen dlite-codegen.c)
target_link_libraries(dlite-codegen
  dlite-static
  dlite-utils-static
  ${extra_link_libraries}
)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  # Additional link libraries on 32-bit systems
  target_link_libraries(dlite-codegen m -pthread)
endif()
target_include_directories(dlite-codegen PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
)


# dlite-env
add_executable(dlite-env dlite-env.c)
target_link_libraries(dlite-env
  dlite-static
  dlite-utils-static
  ${extra_link_libraries}
)
target_include_directories(dlite-env PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
)


# EXECUTABLE RPATH/ RUNPATH: dlite-getuuid dlite-codegen dlite-env
# ================================================================
# 
# This executable will be in three different paths:
# 
# Build
#   ${CMAKE_BINARY_DIR}/tools
# 
# Install
#   ${CMAKE_INSTALL_PREFIX}/bin
#   ${DLITE_PYTHONPATH}/dlite/share/dlite/bin
# 
#   Where:
#     ${DLITE_PYTHONPATH} = ${CMAKE_INSTALL_PREFIX}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages
# 
# The two linked libraries will be copied into the below locations:
#   ${CMAKE_INSTALL_PREFIX}/lib
#   ${DLITE_PYTHONPATH}/dlite
# 
# 
# Two relative paths are thus needed for the install; absolute paths given for build versions.
# The 'bin' versions locate the ../lib libraries; the 'share' versions locate the extension module libraries.
# 
# Note: lib can be lib64 on some Linux platforms e.g. Fedora and SuSE:
#   https://docs.python.org/3/using/configure.html#cmdoption-with-platlibdir

if(APPLE)
  set_target_properties(dlite-getuuid dlite-codegen dlite-env PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "@executable_path/../lib;@executable_path/../../.."
  )
else()
  set_target_properties(dlite-getuuid dlite-codegen dlite-env PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../lib64;$ORIGIN/../../.."
  )
endif()


# Subdirectories
add_subdirectory(tests)


# Install
install(
  TARGETS dlite-getuuid dlite-codegen dlite-env
  DESTINATION bin
)
# Ensure RPATHS updated correctly
install(
  TARGETS dlite-getuuid dlite-codegen dlite-env
  DESTINATION ${Python3_PREFIX}share/dlite/bin
)
install(
  PROGRAMS dlite-docker
  DESTINATION bin
)
install(
  DIRECTORY templates
  DESTINATION ${DLITE_PKG_ROOT}/share/dlite
  PATTERN "*~" EXCLUDE
)
