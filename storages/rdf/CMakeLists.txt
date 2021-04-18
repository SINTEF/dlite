# -*- Mode: cmake -*-
#

set(sources
  dlite-rdf.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-rdf SHARED ${sources})
target_link_libraries(dlite-plugins-rdf
  ${REDLAND_LIBRARIES}
  ${RAPTOR_LIBRARIES}
  dlite
  dlite-utils
  )
target_include_directories(dlite-plugins-rdf PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${REDLAND_INCLUDE_DIRS}
  ${RAPTOR_INCLUDE_DIRS}
  )
if(${REDLAND_DEPENDENCIES})
  add_dependencies(dlite-plugins-rdf ${REDLAND_DEPENDENCIES})
endif()
set_target_properties(dlite-plugins-rdf PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

# Simplify plugin search path for testing in build tree, copy target
# to ${dlite_BINARY_DIR}/plugins
add_custom_command(
  TARGET dlite-plugins-rdf
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-rdf>
    ${dlite_BINARY_DIR}/plugins
  )


install(
  TARGETS dlite-plugins-rdf
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
)

# tests
add_subdirectory(tests)
