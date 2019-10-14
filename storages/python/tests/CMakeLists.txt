# -*- Mode: cmake -*-
#

set(tests
  test_python_storage
  test_postgresql_storage
  )

add_definitions(-DDLITE_ROOT=${dlite_SOURCE_DIR})

# We are linking to dlite-plugins-python DLL - this require that this
# DLL is in the PATH on Windows. Copying the DLL to the current
# BINARY_DIR is a simple way to ensure this.
add_custom_target(
  copy-dlite-plugins-python
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-python>
    ${dlite_BINARY_DIR}/storages/python/tests
  )

# Copy metadata def. to working directory
file(COPY Person.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Compile and add tests
foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test} dlite-plugins-python)
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/storages/python
    ${dlite-src_SOURCE_DIR}/tests
    )
  add_dependencies(${test} copy-dlite-plugins-python)

  add_test(
    NAME ${test}
    COMMAND ${test}
    )

  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGE_PLUGINS=${dlite_STORAGE_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_MAPPING_PLUGINS=${dlite_MAPPING_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_PYTHON_STORAGE_PLUGINS=${dlite_PYTHON_STORAGE_PLUGINS}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_PYTHON_MAPPING_PLUGINS=${dlite_PYTHON_MAPPING_PLUGINS}")
endforeach()


# The test_postgresql_storage test require local configurations of the
# PostgreSQL server.  The test is only enabled if a file pgconf.h can be
# found in the source directory with the following content:
#
#     #define HOST "pg_server_host"
#     #define USER "my_username"
#     #define DATABASE "my_database"
#     #define PASSWORD "my_password"
#
# Depending on how the server is set up, or if you have a ~/.pgpass
# file, PASSWORD can be left undefined.
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pgconf.h)
  set_property(TEST test_postgresql_storage APPEND
    PROPERTY DISABLED True)
endif()
