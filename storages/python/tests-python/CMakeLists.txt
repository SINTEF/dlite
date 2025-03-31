# -*- Mode: cmake -*-
#

set(python-tests
  test_bson_storage_python
  test_yaml_storage_python
  test_mongodb_python
  test_mongodb-atlas_python
  test_redis
  test_minio
  test_template
  test_template-jinja
  test_http
  test_image
  )

# Paths
set(plugindir "${CMAKE_CURRENT_SOURCE_DIR}/../python-storage-plugins")


include(FindPythonModule)

# Disable BSON test if pymongo is not installed
find_python_module(pymongo)
if(NOT PY_PYMONGO)
  add_test(NAME test_bson_storage_python
    COMMAND ${CMAKE_COMMAND} -E echo "disabled (no pymongo)")
  set_property(TEST test_bson_storage_python PROPERTY DISABLED True)
  list(REMOVE_ITEM python-tests test_bson_storage_python)
endif()

# Disable PostgreSQL test if psycopg2 is not installed
find_python_module(psycopg2)
if(NOT PY_PSYCOPG2)
  add_test(NAME test_postgresql_storage_python
    COMMAND ${CMAKE_COMMAND} -E echo "disabled (no psycopg2)")
  set_property(TEST test_postgresql_storage_python PROPERTY DISABLED True)
  list(REMOVE_ITEM python-tests test_postgresql_storage_python)
endif()

# Disable YAML test if PyYAML is not in installed
find_python_module(yaml)
if(NOT PY_YAML)
  add_test(NAME test_yaml_storage_python
    COMMAND ${CMAKE_COMMAND} -E echo "disabled (no PyYAML)")
  set_property(TEST test_yaml_storage_python PROPERTY DISABLED True)
  list(REMOVE_ITEM python-tests test_yaml_storage_python)
endif()


# Add tests
foreach(test ${python-tests})
  add_test(
    NAME ${test}
    COMMAND ${RUNNER} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py
    )

  set_property(TEST ${test} PROPERTY
    ENVIRONMENT "PATH=${dlite_PATH_NATIVE}")
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
    ENVIRONMENT "DLITE_STORAGES=${plugindir}/*.json")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_PYDEBUG=")
  set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "DLITE_USE_BUILD_ROOT=YES")
  if (UNIX AND NOT APPLE)
    set_property(TEST ${test} APPEND PROPERTY
    ENVIRONMENT "LD_LIBRARY_PATH=${dlite_LD_LIBRARY_PATH_NATIVE}")
  endif()
  set_property(TEST ${name} APPEND PROPERTY
    ENVIRONMENT "DLITE_IMPORTSKIP_EXITCODE=$ENV{DLITE_IMPORTSKIP_EXITCODE}")

  # Skip tests that exit with return code 44
  set_property(TEST ${test} PROPERTY SKIP_RETURN_CODE 44)

endforeach()


# Skip minio test if the minio playground storage is full
set_property(TEST test_minio PROPERTY
  SKIP_REGULAR_EXPRESSION "(XMinioStorageFull)|(ConnectTimeoutError)"
)
