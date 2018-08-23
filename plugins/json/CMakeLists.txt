# -*- Mode: cmake -*-
#

set(sources
  ${dlite_SOURCE_DIR}/src/err.c
  ${dlite_SOURCE_DIR}/src/dlite-type.c
  ${dlite_SOURCE_DIR}/src/dlite-entity.c  # FIXME - remove this dependency
  ${uuid_BINARY_DIR}/integers.h
  json-utils.c
  dlite-json-storage.c
  str.c
  vector.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-json ${sources})
target_link_libraries(dlite-json ${JANSSON_LIBRARIES})
target_include_directories(dlite-json PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  ${JANSSON_INCLUDE_DIRS}
  )
if(${JANSSON_DEPENDENCIES})
  add_dependencies(dlite-json ${JANSSON_DEPENDENCIES})
endif()

install(
  TARGETS dlite-json
  DESTINATION lib
)
