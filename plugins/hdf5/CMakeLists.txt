# -*- Mode: cmake -*-
#

set(sources
  dh5lite.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(plugins-hdf5 ${sources})
target_link_libraries(plugins-hdf5
  ${HDF5_LIBRARIES}
  utils
  dlite
  )
target_include_directories(plugins-hdf5 PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite_SOURCE_DIR}/src
  ${dlite_SOURCE_DIR}/src/utils
  ${dlite_BINARY_DIR}/src
  ${HDF5_INCLUDE_DIRS}
  )

if(${HDF5_DEPENDENCIES})
  add_dependencies(plugins-hdf5 ${HDF5_DEPENDENCIES})
endif()

install(
  TARGETS plugins-hdf5
  DESTINATION lib
)
