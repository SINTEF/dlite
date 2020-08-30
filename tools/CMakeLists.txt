# -*- Mode: cmake -*-
project(dlite-tools C)


# dlite-getuuid
add_executable(dlite-getuuid dlite-getuuid.c)
target_link_libraries(dlite-getuuid dlite-static)
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


# dlite-env
add_executable(dlite-env dlite-env.c)
target_link_libraries(dlite-env dlite-static)
target_include_directories(dlite-env PRIVATE
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  )
#add_dependencies(dlite-env ${dlite-src_BINARY_DIR}/config-paths.h)

# Subdirectories
add_subdirectory(tests)


# Install
install(
  TARGETS dlite-getuuid dlite-codegen dlite-env
  DESTINATION bin)

install(
  DIRECTORY templates
  DESTINATION share/dlite
  PATTERN "*~" EXCLUDE
  )
