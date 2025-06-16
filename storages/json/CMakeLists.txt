# -*- Mode: cmake -*-
#

set(sources
  dlite-json-storage.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-json SHARED ${sources})
target_link_libraries(dlite-plugins-json
  dlite
  )
target_include_directories(dlite-plugins-json PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  )

# PLUGIN RPATH/ RUNPATH: dlite-plugins-json
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
  set_target_properties(dlite-plugins-json PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "@loader_path/../../.."
    )
else()
  set_target_properties(dlite-plugins-json PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "$ORIGIN/../../.."
    )
endif()

install(
  TARGETS dlite-plugins-json
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
)

# tests
# For now don't compile json tests if linking with static Python
# There is an issue with library order
if(NOT WITH_STATIC_PYTHON)
  add_subdirectory(tests)
endif()
