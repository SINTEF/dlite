# -*- Mode: cmake -*-
#

set(sources
  dlite-plugins-python.c
  )

#add_definitions(-DHAVE_CONFIG_H)

add_library(dlite-plugins-python SHARED ${sources})
target_link_libraries(dlite-plugins-python
  dlite
  dlite-utils
  )
target_include_directories(dlite-plugins-python PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${dlite-src_SOURCE_DIR}
  ${dlite-src_BINARY_DIR}
  ${Python3_INCLUDE_DIRS}
  )

# PLUGIN RPATH/ RUNPATH: dlite-plugins-python
# ===========================================
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
  set_target_properties(dlite-plugins-python PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "@loader_path/../../.."
    )
else()
  set_target_properties(dlite-plugins-python PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils"
    INSTALL_RPATH "$ORIGIN/../../.."
    )
endif()

install(
  TARGETS dlite-plugins-python
  DESTINATION ${DLITE_STORAGE_PLUGIN_DIRS}
  #LIBRARY DESTINATION lib
)

install(
  DIRECTORY python-storage-plugins
  DESTINATION share/dlite
  PATTERN "*~" EXCLUDE
  PATTERN ".gitignore" EXCLUDE
  )

install(
  FILES
    python-storage-plugins/blob.json
    python-storage-plugins/Image.json
  DESTINATION share/dlite/storages
  )

# tests
# For now don't compile json tests if linking with static Python
# There is an issue with library order
if(NOT WITH_STATIC_PYTHON)
  add_subdirectory(tests-c)
endif()
add_subdirectory(tests-python)

# Fixture tests
set(services
  postgres
  minio
  redis
  mongo
)
foreach(service ${services})
  add_test(
    NAME start_${service}
    COMMAND ${RUNNER} dlite-service --start ${service}
  )
  add_test(
    NAME stop_${service}
    COMMAND ${RUNNER} dlite-service --stop ${service}
  )
  set_tests_properties(start_${service} PROPERTIES FIXTURES_SETUP   ${service})
  set_tests_properties(stop_${service}  PROPERTIES FIXTURES_CLEANUP ${service})

  # Disable cleanup fixture if service is already running
  execute_process(
    COMMAND ${RUNNER} dlite-service ${service}
    RESULT_VARIABLE running
  )
  message("*** running=${running}")
  if(${running} EQUAL 0)
    set_tests_properties(stop_${service} PROPERTIES DISABLED TRUE)
  endif()

endforeach()
