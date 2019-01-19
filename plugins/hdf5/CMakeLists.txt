# -*- Mode: cmake -*-
#

set(sources
  dh5lite.c
  # Can we remove these dependencies?
  ${dlite-src_SOURCE_DIR}/getuuid.c
  ${dlite-src_SOURCE_DIR}/dlite-misc.c
  ${dlite-src_SOURCE_DIR}/dlite-type.c
  #${uuid_SOURCE_DIR}/uuid-static.c
  )

add_definitions(-DHAVE_CONFIG_H)


add_library(dlite-plugins-hdf5 SHARED ${sources})
target_link_libraries(dlite-plugins-hdf5
  dlite-utils
  ${HDF5_LIBRARIES}
  ${UUID_LIBRARIES}
  )
target_include_directories(dlite-plugins-hdf5 PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${uuid_SOURCE_DIR}
  ${uuid_BINARY_DIR}
  ${HDF5_INCLUDE_DIRS}
  )

if(${HDF5_DEPENDENCIES})
  add_dependencies(dlite-plugins-hdf5 ${HDF5_DEPENDENCIES} uuidProj)
endif()

install(
  TARGETS dlite-plugins-hdf5
  DESTINATION lib
)
