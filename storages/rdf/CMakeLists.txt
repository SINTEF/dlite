# -*- Mode: cmake -*-
#

set(sources
  dlite-rdf.c
  )

add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-rdf SHARED ${sources})
target_link_libraries(dlite-plugins-rdf
  ${REDLAND_LIBRARIES}
  ${RAPTOR_LIBRARIES}
  dlite
  dlite-utils
  )
target_include_directories(dlite-plugins-rdf PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${REDLAND_INCLUDE_DIRS}
  ${RAPTOR_INCLUDE_DIRS}
  )

# PLUGIN RPATH/ RUNPATH: dlite-plugins-rdf
# ========================================
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
  set_target_properties(dlite-plugins-rdf PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "@loader_path/../../.."
    )
else()
  set_target_properties(dlite-plugins-rdf PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "$ORIGIN/../../.."
    )
endif()

if(${REDLAND_DEPENDENCIES})
  add_dependencies(dlite-plugins-rdf ${REDLAND_DEPENDENCIES})
endif()
set_target_properties(dlite-plugins-rdf PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  )

# Simplify plugin search path for testing in build tree, copy target
# to ${dlite_BINARY_DIR}/plugins
add_custom_command(
  TARGET dlite-plugins-rdf
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-rdf>
    ${dlite_BINARY_DIR}/plugins
  )


install(
  TARGETS dlite-plugins-rdf
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
)

# tests
add_subdirectory(tests)
