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
target_compile_definitions(dlite-plugins-hdf5 PUBLIC
  H5Literage_vers=1
  H5Oget_info_by_name_vers=1
  H5O_info_t_vers=1
  )
target_include_directories(dlite-plugins-hdf5 PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite_SOURCE_DIR}/src
  ${dlite_BINARY_DIR}/src
  ${HDF5_INCLUDE_DIRS}
  )

# PLUGIN RPATH/ RUNPATH: dlite-plugins-hdf5
# =========================================
# 
# At build, set absolute RPATHS to the other libraries.
# 
# At install the library will be loaded from the directory:
# 
#   ${CMAKE_INSTALL_PREFIX}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/dlite/share/dlite/storage-plugins
#   - OR - using CMake dlite directory variables:
#   ${DLITE_PYTHONPATH}/dlite/share/dlite/storage-plugins
# 
# 
# Needs to locate the libraries in:
# 
#   ${DLITE_PYTHONPATH}/dlite
# 
# 
# The two linked libraries will be copied into the above noted folder at install.

if(APPLE)
  set_target_properties(dlite-plugins-hdf5 PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "@loader_path/../../.."
    )
else()
  set_target_properties(dlite-plugins-hdf5 PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "$ORIGIN/../../.."
    )
endif()

if(${HDF5_DEPENDENCIES})
  add_dependencies(dlite-plugins-hdf5 ${HDF5_DEPENDENCIES} uuidProj)
endif()

set_target_properties(dlite-plugins-hdf5 PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

install(
  TARGETS dlite-plugins-hdf5
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
)
