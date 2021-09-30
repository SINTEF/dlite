# -*- Mode: cmake -*-
#

set(sources
  dlite-json-storage.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-json SHARED ${sources})
target_link_libraries(dlite-plugins-json
  dlite-static
  dlite-utils-static
  )
target_include_directories(dlite-plugins-json PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  )
set_target_properties(dlite-plugins-json PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  BUILD_RPATH "$ORIGIN/../../../"       # library is in ./dlite/share/dlite/storage-plugins, find libdlite-pyembed.so, libdlite.so, libdlite-utils.so in ./dlite
  INSTALL_RPATH "$ORIGIN/../../../"
  )

# Simplify plugin search path for testing in build tree, copy target
# to ${dlite_BINARY_DIR}/plugins
add_custom_command(
  TARGET dlite-plugins-json
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-json>
    ${dlite_BINARY_DIR}/plugins
  )


install(
  TARGETS dlite-plugins-json
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
)

# tests
add_subdirectory(tests)
