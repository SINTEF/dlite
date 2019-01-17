# -*- Mode: cmake -*-
#

set(sources
  dh5lite.c
  )

add_definitions(-DHAVE_CONFIG_H)


add_library(dlite-plugins-hdf5 SHARED ${sources})
target_link_libraries(dlite-plugins-hdf5
  dlite-utils
  ${HDF5_LIBRARIES}
  )
target_include_directories(dlite-plugins-hdf5 PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite_SOURCE_DIR}/src
  ${dlite_SOURCE_DIR}/src/utils
  ${dlite_BINARY_DIR}/src
  ${HDF5_INCLUDE_DIRS}
  )

if(${HDF5_DEPENDENCIES})
  add_dependencies(dlite-plugins-hdf5 ${HDF5_DEPENDENCIES})
endif()

install(
  TARGETS dlite-plugins-hdf5
  DESTINATION lib
)
