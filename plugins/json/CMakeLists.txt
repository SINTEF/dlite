# -*- Mode: cmake -*-
#

set(sources
  json-utils.c
  dlite-json-storage.c
  str.c
  vector.c
  # Can we remove these dependencies?
  #${dlite-src-utils_SOURCE_DIR}/map.c
  #${dlite-src_SOURCE_DIR}/getuuid.c
  #${dlite-src_SOURCE_DIR}/triplestore.c
  #${dlite-src_SOURCE_DIR}/dlite-misc.c
  #${dlite-src_SOURCE_DIR}/dlite-type.c
  #${dlite-src_SOURCE_DIR}/dlite-entity.c
  #${dlite-src_SOURCE_DIR}/dlite-storage.c
  #${dlite-src_SOURCE_DIR}/dlite-arrays.c
  #${dlite-src_SOURCE_DIR}/dlite-store.c
  #${dlite-src_SOURCE_DIR}/dlite-datamodel.c
  #${dlite-src_SOURCE_DIR}/dlite-schemas.c
  #${dlite-src_SOURCE_DIR}/dlite-collection.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-json SHARED ${sources})
target_link_libraries(dlite-plugins-json
  ${JANSSON_LIBRARIES}
  dlite-static-pic
  dlite-utils
  ${UUID_LIBRARIES}
  )
target_include_directories(dlite-plugins-json PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${uuid_BINARY_DIR}
  ${uuid_SOURCE_DIR}
  ${JANSSON_INCLUDE_DIRS}
  )
if(${JANSSON_DEPENDENCIES})
  add_dependencies(plugins-json uuidProj ${JANSSON_DEPENDENCIES})
endif()

install(
  TARGETS dlite-plugins-json
  DESTINATION lib
)
