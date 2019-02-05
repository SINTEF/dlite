# -*- Mode: cmake -*-
#

set(sources
  json-utils.c
  dlite-json-storage.c
  str.c
  vector.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-json SHARED ${sources})
target_link_libraries(dlite-plugins-json
  ${JANSSON_LIBRARIES}
  dlite-static
  dlite-utils-static
  )
target_include_directories(dlite-plugins-json PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${JANSSON_INCLUDE_DIRS}
  )
if(${JANSSON_DEPENDENCIES})
  add_dependencies(plugins-json ${JANSSON_DEPENDENCIES})
endif()
set_target_properties(dlite-plugins-json PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

# Simplify plugin search path for testing in build tree, copy target
# to ${dlite_BINARY_DIR}/plugins
ADD_CUSTOM_COMMAND(
  TARGET dlite-plugins-json
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-json>
    ${dlite_BINARY_DIR}/plugins
  )


install(
  TARGETS dlite-plugins-json
  DESTINATION ${DLITE_STORAGE_PLUGINS_PATH}
)
