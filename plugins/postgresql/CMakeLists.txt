# -*- Mode: cmake -*-
#
set(name postgresql)
set(plugin dlite-plugins-${name})  # target

set(sources
  dlite-${name}-storage.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(${plugin} SHARED ${sources})
target_link_libraries(dlite-plugins-${name}
  dlite-static
  dlite-utils-static
  #${POSTGRESQL_LIBRARIES}
  )
target_include_directories(${plugin} PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  #${POSTGRESQL_INCLUDE_DIRS}
  )
set_target_properties(${plugin} PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )


# Simplify plugin search path for testing in build tree, copy target
# to ${dlite_BINARY_DIR}/plugins
ADD_CUSTOM_COMMAND(
  TARGET ${plugin}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${plugin}>
    ${dlite_BINARY_DIR}/plugins
  )


install(
  TARGETS dlite-plugins-${name}
  DESTINATION ${DLITE_STORAGE_PLUGINS_PATH}
)
