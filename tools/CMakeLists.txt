# -*- Mode: cmake -*-
project(dlite-tools C)

# Workaround for old CMake not setting the correct binary path for MSVC
if(MSVC AND ${CMAKE_VERSION} VERSION_LESS "3.12.10")
  set(dlite-tools_BINARY_DIR ${dlite-tools_BINARY_DIR}/$<CONFIG>)
endif()


# dlite-getuuid
add_executable(dlite-getuuid dlite-getuuid.c)
target_link_libraries(dlite-getuuid
  dlite-static
  )
target_include_directories(dlite-getuuid PRIVATE
  ${dlite_SOURCE_DIR}/src
  )


# dlite-codegen
add_executable(dlite-codegen dlite-codegen.c)
target_link_libraries(dlite-codegen dlite-static)
target_include_directories(dlite-codegen PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  )


# Subdirectories
add_subdirectory(tests)


# Install
install(
  TARGETS dlite-getuuid dlite-codegen
  DESTINATION bin)

install(
  DIRECTORY templates
  DESTINATION share/dlite
  PATTERN "*~" EXCLUDE
  )
