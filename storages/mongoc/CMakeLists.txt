# -*- Mode: cmake -*-
#
set(sources dlite-mongoc.c)

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-mongoc SHARED ${sources})
target_link_libraries(dlite-plugins-mongoc
  dlite
  dlite-utils
  ${MONGOC_LIBRARIES}
  ${BSON_LIBRARIES}
  )
#target_link_libraries (hello_mongoc PRIVATE mongo::mongoc_shared)

target_include_directories(dlite-plugins-mongoc PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${MONGOC_INCLUDE_DIRS}
  ${BSON_INCLUDE_DIRS}
  )

set_target_properties(dlite-plugins-mongoc PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

# Simplify plugin search path for testing in build tree, copy target
# to ${dlite_BINARY_DIR}/plugins
ADD_CUSTOM_COMMAND(
  TARGET dlite-plugins-mongoc
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-mongoc>
    ${dlite_BINARY_DIR}/plugins
  )

install(
  TARGETS dlite-plugins-mongoc
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
)
