# -*- Mode: cmake -*-
#

set(sources
  dlite-plugins-python.c
  )

#add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-python SHARED ${sources})
target_link_libraries(dlite-plugins-python
  dlite
  dlite-utils
  ${Python3_LIBRARIES}
  )
target_include_directories(dlite-plugins-python PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${Python3_INCLUDE_DIRS}
  )
set_target_properties(dlite-plugins-python PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

# Simplify plugin search path for testing in build tree, copy target
# to ${dlite_BINARY_DIR}/plugins
add_custom_command(
  TARGET dlite-plugins-python
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-python>
    ${dlite_BINARY_DIR}/plugins
  )


install(
  TARGETS dlite-plugins-python
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
)

install(
  DIRECTORY python-storage-plugins
  DESTINATION share/dlite
  PATTERN "*~" EXCLUDE
)
# tests
add_subdirectory(tests)
