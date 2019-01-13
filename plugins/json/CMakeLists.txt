# -*- Mode: cmake -*-
#

set(sources
  json-utils.c
  dlite-json-storage.c
  str.c
  vector.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-json ${sources})
target_link_libraries(dlite-plugins-json
  ${JANSSON_LIBRARIES}
  dlite
  dlite-utils
  )
target_include_directories(dlite-plugins-json PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  ${uuid_SOURCE_DIR}
  ${JANSSON_INCLUDE_DIRS}
  )
if(${JANSSON_DEPENDENCIES})
  add_dependencies(plugins-json ${JANSSON_DEPENDENCIES})
endif()

install(
  TARGETS dlite-plugins-json
  DESTINATION lib
)
