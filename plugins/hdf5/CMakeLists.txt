# -*- Mode: cmake -*-
#

set(sources
  dh5lite.c
  # Can we remove these dependencies?
  #${dlite-src_SOURCE_DIR}/getuuid.c
  #${dlite-src_SOURCE_DIR}/dlite-misc.c
  #${dlite-src_SOURCE_DIR}/dlite-type.c
  #${uuid_SOURCE_DIR}/uuid-static.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-hdf5 SHARED ${sources})
target_link_libraries(dlite-plugins-hdf5
  dlite-static
  dlite-utils-static
  ${HDF5_LIBRARIES}
  )
target_include_directories(dlite-plugins-hdf5 PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${HDF5_INCLUDE_DIRS}
  )

if(${HDF5_DEPENDENCIES})
  add_dependencies(dlite-plugins-hdf5 ${HDF5_DEPENDENCIES} uuidProj)
endif()

set_target_properties(dlite-plugins-hdf5 PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

install(
  TARGETS dlite-plugins-hdf5
  DESTINATION ${DLITE_STORAGE_PLUGINS_PATH}
)
