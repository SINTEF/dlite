# -*- Mode: cmake -*-
#

set(sources
  ${dlite_SOURCE_DIR}/src/utils/err.c
  ${dlite_SOURCE_DIR}/src/utils/strtob.c
  ${dlite_SOURCE_DIR}/src/utils/map.c         # FIXME - remove this dependency
  ${dlite_SOURCE_DIR}/src/dlite-type.c        # FIXME - remove this dependency
  ${dlite_SOURCE_DIR}/src/dlite-store.c       # FIXME - remove this dependency
  ${dlite_SOURCE_DIR}/src/dlite-entity.c      # FIXME - remove this dependency
  ${dlite_SOURCE_DIR}/src/dlite-collection.c  # FIXME - remove this dependency
  ${dlite_SOURCE_DIR}/src/triplestore.c       # FIXME - remove this dependency
  #${uuid_BINARY_DIR}/integers.h
  json-utils.c
  dlite-json-storage.c
  str.c
  vector.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(plugins-json ${sources})
target_link_libraries(plugins-json
  ${JANSSON_LIBRARIES}
  )
target_include_directories(plugins-json PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite_SOURCE_DIR}/src
  ${dlite_SOURCE_DIR}/src/utils
  ${dlite_BINARY_DIR}/src
  ${uuid_BINARY_DIR}
  ${uuid_SOURCE_DIR}                          # FIXME - remove this dependency
  ${JANSSON_INCLUDE_DIRS}
  )
if(${JANSSON_DEPENDENCIES})
  add_dependencies(plugins-json ${JANSSON_DEPENDENCIES})
endif()

install(
  TARGETS plugins-json
  DESTINATION lib
)
