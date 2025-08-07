# -*- Mode: cmake -*-
#
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


set(tests
  test_yaml_storage
  test_blob_storage
  test_bson_storage
  test_postgresql_storage
  test_postgresql_storage2
)

add_definitions(
  -DDLITE_ROOT=${dlite_SOURCE_DIR}
  -DCURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
  -DCURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
)

include(FindPythonModule)

# Disable yaml test if PyYAML is not in installed
find_python_module(yaml)
if(NOT PY_YAML)
  add_test(NAME test_yaml_storage
    COMMAND ${CMAKE_COMMAND} -E echo "disabled")
  set_property(TEST test_yaml_storage PROPERTY DISABLED True)
  list(REMOVE_ITEM tests test_yaml_storage)
endif()

# Disable BSON test if pymongo is not installed
find_python_module(pymongo)
if(NOT PY_PYMONGO)
  add_test(NAME test_bson_storage
    COMMAND ${CMAKE_COMMAND} -E echo "disabled")
  set_property(TEST test_bson_storage PROPERTY DISABLED True)
  list(REMOVE_ITEM tests test_bson_storage)
endif()

# Disable postgresql tests if psycopg is not installed or if pgconf.h
# cannot be found
find_python_module(psycopg)
if(NOT PY_PSYCOPG OR NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pgconf.h)
  add_test(NAME test_postgresql_storage
    COMMAND ${CMAKE_COMMAND} -E echo "disabled")
  add_test(NAME test_postgresql_storage2
    COMMAND ${CMAKE_COMMAND} -E echo "disabled")
  set_property(TEST test_postgresql_storage PROPERTY DISABLED True)
  set_property(TEST test_postgresql_storage2 PROPERTY DISABLED True)
  list(REMOVE_ITEM tests test_postgresql_storage)
  list(REMOVE_ITEM tests test_postgresql_storage2)
endif()


# We are linking to dlite-plugins-python DLL - this require that this
# DLL is in the PATH on Windows. Copying the DLL to the current
# BINARY_DIR is a simple way to ensure this.
add_custom_target(
  copy-dlite-plugins-python
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:dlite-plugins-python>
    ${dlite_BINARY_DIR}/storages/python/c-tests
  )

# Copy metadata def. to working directory
file(COPY Person.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Compile and add tests
foreach(test ${tests})
  add_executable(${test} ${test}.c)
  target_link_libraries(${test}
    dlite-plugins-python
    ${extra_link_libraries}
  )
  target_include_directories(${test} PRIVATE
    ${dlite_SOURCE_DIR}/storages/python
    ${dlite-src_SOURCE_DIR}/tests
  )
  add_dependencies(${test} copy-dlite-plugins-python)

  # TEST RPATH/ RUNPATH
  # ===================
  #
  # These tests will be run from the Build directory only.
  # Set absolute RPATHS to the other libraries.
  # TODO: Change all paths to generators e.g. for dlite, dlite-utils etc as easier to maintain

  set_target_properties(${test} PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${dlite_BINARY_DIR}/src;${dlite_BINARY_DIR}/src/utils;$<TARGET_FILE_DIR:dlite-plugins-python>"
    )

  add_test(
    NAME ${test}
    COMMAND ${RUNNER} ${test}
  )


  if(WIN32)
    set_property(TEST ${test} PROPERTY
      ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${dlite_BINARY_DIR}/src")
  endif()
  set_property(TEST ${test} PROPERTY
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<JOIN:${dlite_PATH_NATIVE},\\\;>")
  if (WIN32 AND PYTHON_IS_ANACONDA)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "PYTHONHOME=${Python3_RUNTIME_LIBRARY_DIRS}")
  endif()
  if(MINGW)
    set_property(TEST ${test} APPEND PROPERTY
      ENVIRONMENT "WINEPATH=${dlite_WINEPATH_NATIVE}")
  endif()
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "PYTHONPATH=${dlite_PYTHONPATH_NATIVE}")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")
  if (UNIX AND NOT APPLE)
    set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  endif()
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_STORAGES=${CMAKE_CURRENT_SOURCE_DIR}/*.json|${dlite_SOURCE_DIR}/storages/python/python-storage-plugins/*.json")

  # Skip tests that exit with return code 44
  set_property(TEST ${test} PROPERTY SKIP_RETURN_CODE 44)

endforeach()


# Set dependencies
set_property(TEST test_postgresql_storage2 APPEND
  PROPERTY DEPEND test_postgresql_storage)
