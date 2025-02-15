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

# Subdirectories
add_subdirectory(tests)


# Install
install(
  TARGETS dlite-getuuid dlite-codegen dlite-env
  DESTINATION bin
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
