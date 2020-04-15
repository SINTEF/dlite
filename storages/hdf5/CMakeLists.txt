# -*- Mode: cmake -*-
#

set(sources
  dh5lite.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-hdf5 SHARED ${sources})
target_link_libraries(dlite-plugins-hdf5
  dlite
  dlite-utils
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

# Simplify plugin search path for testing in build tree, copy target
# to ${dlite_BINARY_DIR}/plugins
ADD_CUSTOM_COMMAND(
  TARGET dlite-plugins-hdf5
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-hdf5>
    ${dlite_BINARY_DIR}/plugins
  )

install(
  TARGETS dlite-plugins-hdf5
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
)
